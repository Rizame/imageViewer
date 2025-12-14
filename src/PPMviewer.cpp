#include "PPMviewer.h"
#include <fstream>
#include <iostream>

Glib::RefPtr<Gdk::Pixbuf> PPMviewer::load(const std::string &filepath) {
  std::ifstream image(filepath, std::ios::binary);
  if (!image.is_open()) {
    return nullptr;
  }

  std::string line;
  getline(image, line);
  bool rgbRaw = line == "P6";

  do {
    getline(image, line);
  } while (!line.empty() && line[0] == '#');

  int w, h;
  std::stringstream ss(line);
  ss >> w >> h;

  // color value
  getline(image, line);

  int total_size = w * h * 3;
  std::vector<guint8> m_pixels(total_size);
  if (rgbRaw) {
    image.read(reinterpret_cast<char *>(m_pixels.data()), total_size);
  } else {
  }
  auto pixbuf = Gdk::Pixbuf::create(Gdk::Colorspace::RGB, false, 8, w, h);
  memcpy(pixbuf->get_pixels(), m_pixels.data(), total_size);

  return pixbuf;
}
