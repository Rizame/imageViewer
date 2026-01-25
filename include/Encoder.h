#pragma once
#include <gtkmm.h>
#include <string>

class Encoder {
public:
  virtual ~Encoder() = default;
  virtual bool encodeImage(const Glib::RefPtr<Gdk::Pixbuf> pixbuf,
                           const std::string &newName) = 0;
};
