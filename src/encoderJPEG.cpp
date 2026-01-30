#include "encoderJPEG.h"
#include <fstream>
#include <iostream>

const uint8_t DcLuminanceCodesPerBitsize[16] = {
    0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0}; // sum = 12
const uint8_t DcLuminanceValues[12] = {0, 1, 2, 3, 4,  5,
                                       6, 7, 8, 9, 10, 11}; // => 12 codes
const uint8_t AcLuminanceCodesPerBitsize[16] = {
    0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125}; // sum = 162
const uint8_t AcLuminanceValues[162] =                 // => 162 codes
    {0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06,
     0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1,
     0x08, // 16*10+2 symbols because
     0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72,
     0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27,
     0x28, // upper 4 bits can be 0..F
     0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45,
     0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
     0x59, // while lower 4 bits can be 1..A
     0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75,
     0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88,
     0x89, // plus two special codes 0x00 and 0xF0
     0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3,
     0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5,
     0xB6, // order of these symbols was determined empirically by JPEG
           // committee
     0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
     0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
     0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4,
     0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA};
// Huffman definitions for second DC/AC tables (chrominance / Cb and Cr
// channels)
const uint8_t DcChrominanceCodesPerBitsize[16] = {
    0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0}; // sum = 12
const uint8_t DcChrominanceValues[12] = {
    0, 1, 2, 3, 4,  5,
    6, 7, 8, 9, 10, 11}; // => 12 codes (identical to DcLuminanceValues)
const uint8_t AcChrominanceCodesPerBitsize[16] = {
    0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119}; // sum = 162
const uint8_t AcChrominanceValues[162] =               // => 162 codes
    {0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12,
     0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14,
     0x42, 0x91, // same number of symbol, just different order
     0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0, 0x15, 0x62, 0x72,
     0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19,
     0x1A, 0x26, // (which is more efficient for AC coding)
     0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43,
     0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56,
     0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
     0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83,
     0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95,
     0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
     0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9,
     0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2,
     0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE2, 0xE3, 0xE4,
     0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
     0xF7, 0xF8, 0xF9, 0xFA};

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

struct HuffmanCode {
  uint16_t code;
  int length;
};

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

void createHuffmanTable(HuffmanCode *table,
                        const uint8_t (&CodesPerBitsize)[16],
                        const uint8_t *Values) {
  uint16_t code = 0;
  int countSymbols = 0;
  for (int i = 0; i < 16; i++) {
    for (int k = 0; k < AcLuminanceCodesPerBitsize[i]; k++) {
      table[Values[countSymbols]].code = code;
      table[Values[countSymbols]].length = i + 1;
      countSymbols++;
      code++;
    }
    code = code << 1;
  }
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

void writeBits(unsigned int &buffer, std::ofstream &file, int &bitCount,
               int length, unsigned int input) {}

int computeCategory(int val) {
  if (val == 0)
    return 0;

  val = std::abs(val);
  if (val < 2)
    return 1;
  if (val < 4)
    return 2;
  if (val < 8)
    return 3;
  if (val < 16)
    return 4;
  if (val < 32)
    return 5;
  if (val < 64)
    return 6;
  if (val < 128)
    return 7;
  if (val < 256)
    return 8;
  if (val < 512)
    return 9;
  if (val < 1024)
    return 10;
  if (val < 2048)
    return 11;
  if (val < 4096)
    return 12;
  if (val < 8192)
    return 13;
  if (val < 16384)
    return 14;
  return 0;
}

int huffmanEncoding(const int (&input)[64], std::ofstream &file, int &last_dc,
                    unsigned int &bitBuffer, int &bitCount) {}

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

  unsigned int bitBuffer;
  int bitCount = 0;

  outputFile.put(0xFF);
  outputFile.put(0xD8);
  // DCT -> quantization
  //

  HuffmanCode ACtableY[256];
  HuffmanCode DCtableY[16];
  HuffmanCode ACtableCh[256];
  HuffmanCode DCtableCh[16];

  createHuffmanTable(ACtableY, AcLuminanceCodesPerBitsize, AcLuminanceValues);
  createHuffmanTable(DCtableY, DcLuminanceCodesPerBitsize, DcLuminanceValues);
  createHuffmanTable(ACtableCh, AcChrominanceCodesPerBitsize,
                     AcChrominanceValues);
  createHuffmanTable(DCtableCh, DcChrominanceCodesPerBitsize,
                     DcChrominanceValues);

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
          huffmanEncoding(flattened, outputFile, last_dc_l, bitBuffer,
                          bitCount);
        }
      }
      fetch8x8Block(m_cb, input, w / 2, h / 2, x / 2, y / 2);
      computeDCT(input, dct);
      quantizeBlock(dct, Q_CHROMINANCE, quantized);
      zigzagFlattening(quantized, flattened);
      huffmanEncoding(flattened, outputFile, last_dc_cb, bitBuffer, bitCount);

      fetch8x8Block(m_cr, input, w / 2, h / 2, x / 2, y / 2);
      computeDCT(input, dct);
      quantizeBlock(dct, Q_CHROMINANCE, quantized);
      zigzagFlattening(quantized, flattened);
      huffmanEncoding(flattened, outputFile, last_dc_cr, bitBuffer, bitCount);
    }
  }
  outputFile.close();

  return true;
}
