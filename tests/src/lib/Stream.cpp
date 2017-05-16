#include "Stream.h"
#include "trace.h"
#include <iostream>
#include <Arduino.h>

Stream::Stream() :
  _expectBuffer(new Buffer()),
  _error(false),
  _written(0)
{}

Stream::~Stream() {
  delete _expectBuffer;
}

size_t Stream::write(uint8_t b)  {
  _written++;

  TRACE(std::hex << (unsigned int)b);
  if (_expectBuffer->available()) {
    uint8_t expected = _expectBuffer->next();
    if (expected != b) {
      _error = true;
      TRACE("!=" << (unsigned int)expected);
    }
  } else
    _error = true;
  TRACE("\n"<< std::dec);

  return 1;
}
