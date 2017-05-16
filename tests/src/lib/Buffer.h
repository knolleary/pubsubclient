#ifndef buffer_h
#define buffer_h

#include "Arduino.h"

class Buffer {
private:
  uint8_t _buffer[1024];
  size_t _pos, _length;
    
public:
  Buffer();
  Buffer(uint8_t* buf, size_t size) { add(buf, size); }
    
  virtual int available() const { return _length - _pos; }
  virtual uint8_t next();
  virtual void reset() { _pos = 0; }
    
  virtual void add(uint8_t* buf, size_t size);
};

#endif
