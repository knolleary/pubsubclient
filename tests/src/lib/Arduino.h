#ifndef Arduino_h
#define Arduino_h

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


extern "C"{
    typedef uint8_t byte ;
    typedef uint8_t boolean ;

    /* sketch */
    extern void setup( void ) ;
    extern void loop( void ) ;
    uint32_t millis( void );
    uint8_t pgm_read_byte_near(uint8_t*);
#define PROGMEM 
}

#endif // Arduino_h
