# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ARCHITECTURE = avr
BOARD_TAG    = pro
BOARD_SUB	 = 8MHzatmega328
MONITOR_BAUDRATE  = 115200
ARDUINO_LIBS = RF24 RF24Ethernet RF24Mesh RF24Network PubSubClient SPI

CFLAGS_STD = -std=gnu11
CXXFLAGS_STD = -std=gnu++11
CXXFLAGS += -pedantic -Wall -Wextra -fno-exceptions -Os

include /opt/Arduino-Makefile/Arduino.mk
