#pragma once
#include <string>

class Encoder {
public:
  virtual ~Encoder() = default;
  virtual bool encodeImage(const std::string &fileName,
                           const std::string &newName) = 0;
};
