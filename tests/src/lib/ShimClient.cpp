#include "ShimClient.h"
#include "trace.h"
#include <iostream>
#include <Arduino.h>
#include <ctime>

extern "C" {
  uint32_t millis(void) {
    return time(0)*1000;
  }
}

ShimClient::ShimClient() :
  _responseBuffer(new Buffer()),
  _expectBuffer(new Buffer()),
  _allowConnect(true),
  _connected(false),
  _error(false),
  _expectAnything(true),
  _received(0),
  _expectedPort(0)
{}

ShimClient::~ShimClient() {
  delete _responseBuffer;
  delete _expectBuffer;
}

int ShimClient::connect(IPAddress ip, uint16_t port) {
  if (_allowConnect)
    _connected = true;

  if (_expectedPort != 0) {
    if (ip != _expectedIP) {
      TRACE( "ip mismatch\n");
      _error = true;
    }
    if (port != _expectedPort) {
      TRACE( "port mismatch\n");
      _error = true;
    }
  }

  return _connected;
}
int ShimClient::connect(const char *host, uint16_t port)  {
  if (_allowConnect)
    _connected = true;

  if (_expectedPort != 0) {
    if (strcmp(host, _expectedHost) != 0) {
      TRACE( "host mismatch\n");
      _error = true;
    }
    if (port != _expectedPort) {
      TRACE( "port mismatch\n");
      _error = true;
    }
    
  }

  return _connected;
}
size_t ShimClient::write(uint8_t b)  {
  _received++;

  TRACE(std::hex << (unsigned int)b);
  if (!_expectAnything)
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
size_t ShimClient::write(const uint8_t *buf, size_t size)  {
  _received += size;

  TRACE( "[" << std::dec << (unsigned int)(size) << "] ");
  for (size_t i = 0; i < size; i++) {
    if (i > 0)
      TRACE(":");
    TRACE(std::hex << (unsigned int)(buf[i]));
        
    if (!_expectAnything)
      if (_expectBuffer->available()) {
	uint8_t expected = _expectBuffer->next();
	if (expected != buf[i]) {
	  _error = true;
	  TRACE("!=" << (unsigned int)expected);
	}
      } else
	_error = true;
  }
  TRACE("\n"<<std::dec);

  return size;
}

int ShimClient::read(uint8_t *buf, size_t size) { 
  for (size_t i = 0; i < size; i++)
    buf[i] = read();
  return size;
}

ShimClient* ShimClient::respond(uint8_t *buf, size_t size) {
  _responseBuffer->add(buf,size);
  return this;
}

ShimClient* ShimClient::expect(uint8_t *buf, size_t size) {
  _expectAnything = false;
  _expectBuffer->add(buf,size);
  return this;
}

void ShimClient::expectConnect(IPAddress ip, uint16_t port) {
  _expectedIP = ip;
  _expectedPort = port;
}

void ShimClient::expectConnect(const char *host, uint16_t port) {
  _expectedHost = host;
  _expectedPort = port;
}

