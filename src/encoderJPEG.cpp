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

static const int ZIGZAG_ORDER[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 41, 48, 56, 49, 42, 35, 28, 21, 13, 6,  7,  14,
    20, 27, 34, 43, 50, 57, 58, 51, 44, 37, 29, 22, 15, 23, 30, 38,
    45, 52, 59, 60, 53, 46, 39, 31, 47, 54, 61, 62, 55, 63};

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
void zigzagFlattening(const int (&input)[8][8], int (&output)[64]) {
  for (int i = 0; i < 64; i++) {
    int z = ZIGZAG_ORDER[i];
    int y = z / 8;
    int x = z % 8;
    output[i] = input[y][x];
  }
}

int huffmanEncoding(const int (&input)[64], std::ofstream &file, int last_dc) {
  return 0;
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
  double dct[8][8];
  int quantized[8][8];
  int flattened[64];

  int last_dc_l = 0;
  int last_dc_cb = 0;
  int last_dc_cr = 0;
  std::ofstream outputFile("encoded.jpeg");
  // CREATE AND FILL THE HEADER OF THE JPEG

  // DCT -> quantization
  for (int y = 0; y < h; y += 16) {
    for (int x = 0; x < w; x += 16) {

      for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
          int px = x + (dx * 8);
          int py = y + (dy * 8);
          fetch8x8Block(m_l, input, w, h, px, py);
          computeDCT(input, dct);
          quantizeBlock(dct, Q_LUMINANCE, quantized);
          zigzagFlattening(quantized, flattened);
          huffmanEncoding(flattened, outputFile, last_dc_l);
          last_dc_l = flattened[0];
        }
      }
      fetch8x8Block(m_cb, input, w / 2, h / 2, x / 2, y / 2);
      computeDCT(input, dct);
      quantizeBlock(dct, Q_CHROMINANCE, quantized);
      zigzagFlattening(quantized, flattened);
      huffmanEncoding(flattened, outputFile, last_dc_cb);
      last_dc_cb = flattened[0];

      fetch8x8Block(m_cr, input, w / 2, h / 2, x / 2, y / 2);
      computeDCT(input, dct);
      quantizeBlock(dct, Q_CHROMINANCE, quantized);
      zigzagFlattening(quantized, flattened);
      huffmanEncoding(flattened, outputFile, last_dc_cr);
      last_dc_cr = flattened[0];
    }
  }
  outputFile.close();

  return true;
}
