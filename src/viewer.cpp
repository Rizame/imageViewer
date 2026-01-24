#include "viewer.h"
#include "Encoder.h"
#include "encodeFactory.h"
#include "loaderFactory.h"
#include <gtkmm.h>
#include <iostream>

Viewer::Viewer(int w, int h) {
  set_title("Basic application");
  set_default_size(w, h);
}

void Viewer::load_file(const std::string &filename) {
  std::string path;
  if (filename[0] != '/') {
    path = "/home/risame/study/cpp/imageViewer/images/" + filename;
  } else
    path = filename;

  auto loader = create_loader(path);
  if (!loader)
    return;

  auto pixbuf = loader->load(path);
  if (!pixbuf) {
    std::cerr << "Failed to load: " << path << std::endl;
    return;
  }

  if (!m_image) {
    m_image = std::make_unique<Gtk::Image>();
    // set_child(*m_image);
  }

  m_image->set(pixbuf);

  // set_default_size(pixbuf->get_width(), pixbuf->get_height());

  auto encoder = create_encoder("testEncode.jpeg");
  if (!encoder)
    return;

  encoder->encodeImage(pixbuf, "testEncode.jpeg");
}
