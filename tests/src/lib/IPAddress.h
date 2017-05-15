#ifndef IPAddress_h
#define IPAddress_h

class IPAddress {
private:
  uint8_t _octets[4];

public:
  IPAddress() :
    _octets{0, 0, 0, 0}
  {}

  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) :
    _octets{a, b, c, d}
  {}

  IPAddress(const IPAddress& other) = default;

  IPAddress& operator=(const IPAddress& other) = default;

  bool operator !=(const IPAddress& b) {
    return (_octets[0] != b._octets[0])
      && (_octets[1] != b._octets[1])
      && (_octets[2] != b._octets[2])
      && (_octets[3] != b._octets[3]);
  }
};


#endif
