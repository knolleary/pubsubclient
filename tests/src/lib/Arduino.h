#ifndef Arduino_h
#define Arduino_h

#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef std::string String;

extern "C"{
    typedef uint8_t byte ;
    typedef uint8_t boolean ;

    /* sketch */
    extern void setup( void ) ;
    extern void loop( void ) ;
    uint32_t millis( void );
}

#define PROGMEM
#define PGM_P         const char *
#define PGM_VOID_P    const void *
#define FPSTR(p)      ((const char *)(p))
#define PSTR(s)       (s)
#define _SFR_BYTE(n)  (n)

#define pgm_read_byte(addr)   (*(const unsigned char *)(addr))
#define pgm_read_word(addr)   (*(const unsigned short *)(addr))
#define pgm_read_dword(addr)  (*(const unsigned long *)(addr))
#define pgm_read_float(addr)  (*(const float *)(addr))

#define pgm_read_byte_near(addr)  pgm_read_byte(addr)
#define pgm_read_word_near(addr)  pgm_read_word(addr)
#define pgm_read_dword_near(addr) pgm_read_dword(addr)
#define pgm_read_float_near(addr) pgm_read_float(addr)
#define pgm_read_byte_far(addr)   pgm_read_byte(addr)
#define pgm_read_word_far(addr)   pgm_read_word(addr)
#define pgm_read_dword_far(addr)  pgm_read_dword(addr)
#define pgm_read_float_far(addr)  pgm_read_float(addr)


class __FlashStringHelper;

inline void yield(void) {}

#endif // Arduino_h
