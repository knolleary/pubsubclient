#ifndef Stream_h
#define Stream_h

#include "Arduino.h"
#include "Buffer.h"

class Stream {
private:
  Buffer* _expectBuffer;
  bool _error;
  size_t _written;

public:
  Stream();
  virtual ~Stream();

  virtual size_t write(uint8_t);
    
  virtual bool error(void) const { return _error; }
  virtual void expect(uint8_t *buf, size_t size) { _expectBuffer->add(buf, size); }
  virtual size_t length(void) const { return _written; }
};

#endif
