#ifndef buffer_h
#define buffer_h

#include "Arduino.h"
#include <string>

class Buffer {
private:
  std::string _buffer;
  size_t _pos;
    
public:
  Buffer();
  Buffer(uint8_t* buf, size_t size) { add(buf, size); }
    
  virtual size_t available() const { return _buffer.length() - _pos; }
  virtual int next();
  virtual void reset() { _pos = 0; }
    
  virtual void add(uint8_t* buf, size_t size);
};

#endif
