#pragma once
#include "Encoder.h"
#include "encoderJPEG.h"
#include <iostream>
#include <memory>
#include <string>

std::unique_ptr<Encoder> create_encoder(const std::string &filename) {
  if (filename.ends_with(".jpeg"))
    return std::make_unique<encoderJPEG>();
  std::cerr << "Unimplemented encoding extension";
  return nullptr;
}
