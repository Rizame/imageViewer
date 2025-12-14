// ImageLoaderFactory.h
#pragma once
#include "PPMviewer.h"
#include "loader.h"
#include <iostream>
#include <memory>
#include <string>

std::unique_ptr<ImageLoader> create_loader(const std::string &filename) {
  if (filename.ends_with(".ppm"))
    return std::make_unique<PPMviewer>();
  std::cerr << "Unimplemented extenstion";
  return nullptr;
}
