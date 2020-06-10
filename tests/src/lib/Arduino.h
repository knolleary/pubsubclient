#ifndef Arduino_h
#define Arduino_h

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>
#include "Print.h"


extern "C"{
    typedef uint8_t byte ;
    typedef uint8_t boolean ;

    /* sketch */
    extern void setup( void ) ;
    extern void loop( void ) ;
    uint32_t millis( void );
}

#define PROGMEM
#define pgm_read_byte_near(x) *(x)

#define strnlen_P(x,y) strnlen(x,y)

#define yield(x) {}

/** Mock for F()-defined Strings */
class __FlashStringHelper : private std::string
{
  public:

      __FlashStringHelper(const char * pStr) : std::string(pStr) {}
      const char * c_str() const { return std::string::c_str(); }
      operator const char * () const { return std::string::c_str(); }
};

#define PGM_P const char *


#endif // Arduino_h
