#pragma once

#include <Encoder.h>
#include <gtkmm.h>

class encoderJPEG : public Encoder {
public:
  bool encodeImage(const Glib::RefPtr<Gdk::Pixbuf> pixbuf,
                   const std::string &newName) override;
};
