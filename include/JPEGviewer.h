#pragma once

#include <gtkmm.h>
#include <loader.h>

class JPEGviewer : public ImageLoader {
public:
  Glib::RefPtr<Gdk::Pixbuf> load(const std::string &path) override;
};
