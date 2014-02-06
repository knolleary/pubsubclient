#include "ShimClient.h"
#include "trace.h"
#include <iostream>
#include <Arduino.h>
#include <ctime>

extern "C" {
    uint32_t millis(void) {
       return time(0)*1000;
    }
    uint8_t pgm_read_byte_near(uint8_t*) {
       return 0;
    }
}

ShimClient::ShimClient() {
    this->responseBuffer = new Buffer();
    this->expectBuffer = new Buffer();
    this->_allowConnect = true;
    this->_connected = false;
    this->_receivedExpected = true;
    this->expectAnything = true;
    this->_received = 0;
}

int ShimClient::connect(IPAddress ip, uint16_t port) {
    if (this->_allowConnect) {
        this->_connected = true;
    }
    return this->_connected;
}
int ShimClient::connect(const char *host, uint16_t port)  {
    if (this->_allowConnect) {
        this->_connected = true;
    }
    return this->_connected;
}
size_t ShimClient::write(uint8_t b)  { std::cout << "!!not implemented!! " << b; return 1; }
size_t ShimClient::write(const uint8_t *buf, size_t size)  {
    this->_received += size;
    TRACE( "[" << std::dec << (unsigned int)(size) << "] ");
    uint16_t i=0;
    for (;i<size;i++) {
        if (i>0) {
            TRACE(":");
        }
        TRACE(std::hex << (unsigned int)(buf[i]));
        
        if (!this->expectAnything) {
            if (this->expectBuffer->available()) {
                uint8_t expected = this->expectBuffer->next();
                if (expected != buf[i]) {
                    TRACE("!=" << (unsigned int)expected);
                    this->_receivedExpected = false;
                }
            } else {
                this->_receivedExpected = false;
            }
        }
    }
    TRACE("\n");
    return size;
}
int ShimClient::available()  {
    return this->responseBuffer->available();
}
int ShimClient::read()  { return this->responseBuffer->next(); }
int ShimClient::read(uint8_t *buf, size_t size) { 
    uint16_t i = 0;
    for (;i<size;i++) {
        buf[i] = this->read();
    }
    return size;
}
int ShimClient::peek()  { return 0; }
void ShimClient::flush() {}
void ShimClient::stop() {
    this->setConnected(false);
}
uint8_t ShimClient::connected() { return this->_connected; }
ShimClient::operator bool() { return true; }


ShimClient* ShimClient::respond(uint8_t *buf, size_t size) {
    this->responseBuffer->add(buf,size);
    return this;
}

ShimClient* ShimClient::expect(uint8_t *buf, size_t size) {
    this->expectAnything = false;
    this->expectBuffer->add(buf,size);
    return this;
}

void ShimClient::setConnected(bool b) {
    this->_connected = b;
}
void ShimClient::setAllowConnect(bool b) {
    this->_allowConnect = b;
}

bool ShimClient::receivedExpected() {
    return !this->expectBuffer->available() && this->_receivedExpected;
}

uint16_t ShimClient::received() {
    return this->_received;
}
