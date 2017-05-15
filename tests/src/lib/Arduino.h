#ifndef Arduino_h
#define Arduino_h

#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef std::string String;

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

#define SIZE_IRRELEVANT 0x7fffffff

extern "C"{
    typedef uint8_t byte ;
    typedef uint8_t boolean ;

    /* sketch */
    extern void setup( void ) ;
    extern void loop( void ) ;
    uint32_t millis( void );

  inline void* memcpy_P(void* dest, PGM_VOID_P src, size_t count) {
    const uint8_t* read = reinterpret_cast<const uint8_t*>(src);
    uint8_t* write = reinterpret_cast<uint8_t*>(dest);

    while (count) {
      *write++ = pgm_read_byte(read++);
      count--;
    }

    return dest;
  }

  inline char* strncpy_P(char* dest, PGM_P src, size_t size) {
    const char* read = src;
    char* write = dest;
    char ch = '.';
    while (size > 0 && ch != '\0') {
      ch = pgm_read_byte(read++);
      *write++ = ch;
      size--;
    }

    return dest;
  }

  inline size_t strnlen_P(PGM_P s, size_t size) {
    const char* cp;
    for (cp = s; size != 0 && pgm_read_byte(cp) != '\0'; cp++, size--);
    return (size_t) (cp - s);
  }
}

#define strcpy_P(dest, src)          strncpy_P((dest), (src), SIZE_IRRELEVANT)

#define strlen_P(strP)          strnlen_P((strP), SIZE_IRRELEVANT)

class __FlashStringHelper;

inline void yield(void) {}

#endif // Arduino_h
