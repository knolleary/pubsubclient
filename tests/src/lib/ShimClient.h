#ifndef shimclient_h
#define shimclient_h

#include "Arduino.h"
#include "Client.h"
#include "IPAddress.h"
#include "Buffer.h"


class ShimClient : public Client {
private:
  Buffer* _responseBuffer;
  Buffer* _expectBuffer;
  bool _allowConnect;
  bool _connected;
  bool _expectAnything;
  bool _error;
  size_t _received;
  IPAddress _expectedIP;
  uint16_t _expectedPort;
  const char* _expectedHost;

public:
  ShimClient();
  virtual ~ShimClient();

  virtual int connect(IPAddress ip, uint16_t port);
  virtual int connect(const char *host, uint16_t port);
  virtual size_t write(uint8_t);
  virtual size_t write(const uint8_t *buf, size_t size);
  virtual int available(void) { return _responseBuffer->available(); }
  virtual int read(void) { return _responseBuffer->next(); }
  virtual int read(uint8_t *buf, size_t size);
  virtual int peek(void) { return 0; }
  virtual void flush(void) {}
  virtual void stop(void) { setConnected(false); }
  virtual uint8_t connected(void) { return _connected; }
  virtual operator bool(void) { return true; }
  
  virtual ShimClient* respond(uint8_t *buf, size_t size);
  virtual ShimClient* expect(uint8_t *buf, size_t size);
  
  virtual void expectConnect(IPAddress ip, uint16_t port);
  virtual void expectConnect(const char *host, uint16_t port);
  
  virtual size_t received(void) const { return _received; }
  virtual bool error(void) const { return _error; }
  
  virtual void setAllowConnect(bool b) { _allowConnect = b; }
  virtual void setConnected(bool b) { _connected = b; }
};

#endif
