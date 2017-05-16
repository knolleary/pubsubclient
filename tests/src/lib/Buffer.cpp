#include "Buffer.h"
#include "Arduino.h"

Buffer::Buffer() :
  _pos(0), _length(0)
{}

uint8_t Buffer::next() {
  if (available())
    return _buffer[_pos++];

  return 0;
}

void Buffer::add(uint8_t* buf, size_t size) {
  for (size_t i = 0;i < size; i++)
    _buffer[_length++] = buf[i];
}
