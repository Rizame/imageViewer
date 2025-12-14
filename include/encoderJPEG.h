#pragma once

#include <Encoder.h>
#include <gtkmm.h>

class encoderJPEG : public Encoder {
public:
  bool encodeImage(const std::string &fileName,
                   const std::string &newName) override;
};
