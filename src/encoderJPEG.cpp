#include "encoderJPEG.h"
#include <fstream>
#include <iostream>

static const int Q_LUMINANCE[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},     {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},     {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},   {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101}, {72, 92, 95, 98, 112, 100, 103, 99}};

static const int Q_CHROMINANCE[8][8] = {
    {17, 18, 24, 47, 99, 99, 99, 99}, {18, 21, 26, 66, 99, 99, 99, 99},
    {24, 26, 56, 99, 99, 99, 99, 99}, {47, 66, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}, {99, 99, 99, 99, 99, 99, 99, 99},
    {99, 99, 99, 99, 99, 99, 99, 99}, {99, 99, 99, 99, 99, 99, 99, 99}};

void computeDCT(const double (&input)[8][8], double (&output)[8][8]) {

  for (int v = 0; v < 8; v++) {
    for (int u = 0; u < 8; u++) {
      double sum = 0.0;
      for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
          double cosX = std::cos(((2.0 * x + 1.0) * u * M_PI) / 16.0);
          double cosY = std::cos(((2.0 * y + 1.0) * v * M_PI) / 16.0);
          double pixelValue = input[y][x];

          sum += pixelValue * cosX * cosY;
        }
      }
      double cu = (u == 0) ? (1.0 / std::sqrt(2.0)) : 1.0;
      double cv = (v == 0) ? (1.0 / std::sqrt(2.0)) : 1.0;
      output[v][u] = 0.25f * cu * cv * sum;
    }
  }

  std::cout << "\nDCT computed";
}

void chrominanceDownsampling(guint8 *m_pixels, std::vector<guint8> m_l,
                             std::vector<guint8> m_cb, std::vector<guint8> m_cr,
                             int w, int h) {
  //// RGB -> Y, Cb, Cr
  for (int y = 0; y < h; y += 2) {
    for (int x = 0; x < w; x += 2) {
      int cb_avg = 0;
      int cr_avg = 0;

      for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
          int px = x + dx;
          int py = y + dy;

          int idx = (py * w + px) * 3;
          int R = m_pixels[idx];
          int G = m_pixels[idx + 1];
          int B = m_pixels[idx + 2];

          m_l[py * w + px] =
              static_cast<guint8>(0.299 * R + 0.587 * G + 0.114 * B);
          cb_avg +=
              static_cast<guint8>(-0.1687 * R - 0.3313 * G + 0.5 * B + 128);
          cr_avg +=
              static_cast<guint8>(0.5 * R - 0.4187 * G - 0.0813 * B + 128);
        }
      }

      int k = y / 2 * (w / 2) + x / 2;
      m_cb[k] = cb_avg / 4.0;
      m_cr[k] = cr_avg / 4.0;
    }
  }
}

void fetch8x8Block(const std::vector<guint8> &source,
                   double (&destination)[8][8], int w, int h, int startX,
                   int startY) {
  for (int dy = 0; dy < 8; dy++) {
    for (int dx = 0; dx < 8; dx++) {
      int imgX = startX + dx;
      int imgY = startY + dy;
      if (imgX < w && imgY < h) {
        int idx = imgY * w + imgX;
        destination[dy][dx] = static_cast<double>(source[idx]) - 128.0;
      } else {
        destination[dy][dx] = 0.0;
      }
    }
  }
}

void quantizeBlock(const double data[8][8], const int table[8][8],
                   int (&out)[8][8]) {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      double result = data[y][x] / table[y][x];
      out[y][x] = static_cast<int>(std::round(result));
    }
  }
}

bool encoderJPEG::encodeImage(const Glib::RefPtr<Gdk::Pixbuf> pixbuf,
                              const std::string &newName) {

  int w = pixbuf->get_width();
  int h = pixbuf->get_height();

  int total_size = w * h * 3;
  guint8 *m_pixels = new guint8[total_size];
  std::vector<guint8> m_l(w * h);
  std::vector<guint8> m_cb(w * h / 4);
  std::vector<guint8> m_cr(w * h / 4);

  memcpy(m_pixels, pixbuf->get_pixels(), total_size);
  chrominanceDownsampling(m_pixels, m_l, m_cb, m_cr, w, h);
  delete[] m_pixels;
  double input[8][8];
  double output[8][8];
  int quantized[8][8];

  for (int y = 0; y < h; y += 16) {
    for (int x = 0; x < w; x += 16) {

      for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
          int px = x + (dx * 8);
          int py = y + (dy * 8);
          fetch8x8Block(m_l, input, w, h, px, py);
          computeDCT(input, output);
          quantizeBlock(output, Q_LUMINANCE, quantized);
        }
      }
      fetch8x8Block(m_cb, input, w / 2, h / 2, x / 2, y / 2);
      computeDCT(input, output);
      quantizeBlock(output, Q_CHROMINANCE, quantized);

      fetch8x8Block(m_cr, input, w / 2, h / 2, x / 2, y / 2);
      computeDCT(input, output);
      quantizeBlock(output, Q_CHROMINANCE, quantized);
    }
  }
  // DCT -> quantization

  return true;
}
