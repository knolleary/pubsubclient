#include "Buffer.h"
#include "Arduino.h"

Buffer::Buffer() :
  _pos(0)
{}

int Buffer::next() {
  if (available())
    return _buffer[_pos++];

  return -1;
}

void Buffer::add(uint8_t* buf, size_t size) {
  for (size_t i = 0;i < size; i++)
    _buffer += buf[i];
}
