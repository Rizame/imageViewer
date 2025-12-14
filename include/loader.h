#pragma once
#include <gtkmm.h>
#include <memory.h>

class ImageLoader {
public:
  virtual ~ImageLoader() = default;
  virtual Glib::RefPtr<Gdk::Pixbuf> load(const std::string &path) = 0;
};
