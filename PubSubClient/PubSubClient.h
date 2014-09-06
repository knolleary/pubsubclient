/*
 PubSubClient.h - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#include <Arduino.h>
#include "Client.h"
#include "Stream.h"

#define MQTT_DEBUG
#ifdef MQTT_DEBUG
  #ifdef PSTR
   #define WRITEF( ... ) Serial.write( F(__VA_ARGS__) )
   #define PRINTLNF( ... ) Serial.println( F(__VA_ARGS__) )
   #define PRINTF( ... ) Serial.print( F(__VA_ARGS__) )
  #else
   #define WRITEF( ... ) Serial.write( __VA_ARGS__ )
   #define PRINTLNF( ... ) Serial.println( __VA_ARGS__ )
   #define PRINTF( ... ) Serial.print( __VA_ARGS__ )
  #endif
   #define WRITE( ... ) Serial.write( __VA_ARGS__ )
   #define PRINTLN( ... ) Serial.println( __VA_ARGS__ )
   #define PRINT( ... ) Serial.print( __VA_ARGS__ )
   #define PRINTCH( ... ) Serial.print( __VA_ARGS__ )
#else
   #define PRINTCH( ... )
   #define WRITE( ... )
   #define PRINTLN( ... )
   #define PRINT( ... )
#endif

// MQTT_MAX_PACKET_SIZE : Maximum packet size
#define MQTT_MAX_PACKET_SIZE 128

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#define MQTT_KEEPALIVE 15

#define MQTTPROTOCOLVERSION 3
#define MQTTCONNECT     1 << 4  // Client request to connect to Server
#define MQTTCONNACK     2 << 4  // Connect Acknowledgment
#define MQTTPUBLISH     3 << 4  // Publish message
#define MQTTPUBACK      4 << 4  // Publish Acknowledgment
#define MQTTPUBREC      5 << 4  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6 << 4  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7 << 4  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8 << 4  // Client Subscribe request
#define MQTTSUBACK      9 << 4  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 << 4 // Client Unsubscribe request
#define MQTTUNSUBACK    11 << 4 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 << 4 // PING Request
#define MQTTPINGRESP    13 << 4 // PING Response
#define MQTTDISCONNECT  14 << 4 // Client is Disconnecting
#define MQTTReserved    15 << 4 // Reserved

#define MQTTQOS0        (0 << 1)
#define MQTTQOS1        (1 << 1)
#define MQTTQOS2        (2 << 1)

typedef enum {
  MQTTSTATUS_ACCEPTED,
  MQTTSTATUS_UNACCEPTABLE_PROTOCOL_VERSION,
  MQTTSTATUS_IDENTIFIER_REJECTED,
  MQTTSTATUS_SERVER_UNAVAILABLE,
  MQTTSTATUS_CREDENTIALS_REFUSED,
  MQTTSTATUS_UNAUTHORIZED
}ps_status_t;

class PubSubClient {
private:
   Client* _client;
   uint8_t buffer[MQTT_MAX_PACKET_SIZE];
   uint16_t nextMsgId;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;
   void (*callback)(char*,uint8_t*,unsigned int);
   uint16_t readPacket(uint8_t*);
   uint8_t readByte();
   boolean write(uint8_t header, uint8_t* buf, uint16_t length);
   boolean write(uint8_t header, uint8_t* buf, uint16_t length, bool sendData);
   uint16_t writeString(char* string, uint8_t* buf, uint16_t pos);
   uint8_t *ip;
   char* domain;
   uint16_t port;
   Stream* stream;
   uint8_t pubsubStatus;
public:
   PubSubClient();
   PubSubClient(uint8_t *, uint16_t, void(*)(char*,uint8_t*,unsigned int),Client& client);
   PubSubClient(uint8_t *, uint16_t, void(*)(char*,uint8_t*,unsigned int),Client& client, Stream&);
   PubSubClient(char*, uint16_t, void(*)(char*,uint8_t*,unsigned int),Client& client);
   PubSubClient(char*, uint16_t, void(*)(char*,uint8_t*,unsigned int),Client& client, Stream&);
   boolean connect(char *);
   boolean connect(char *, char *, char *);
   boolean connect(char *, char *, uint8_t, uint8_t, char *);
   boolean connect(char *, char *, char *, char *, uint8_t, uint8_t, char*);
   void disconnect();
   uint8_t status();
   boolean publish(char *, char *);
   boolean publish(char *, uint8_t *, unsigned int);
   boolean publish(char *, uint8_t *, unsigned int, boolean);
   boolean publishHeader(char* topic, unsigned int plength, boolean retained);
   boolean publish_P(char *, uint8_t PROGMEM *, unsigned int, boolean);
   boolean subscribe(char *);
   boolean subscribe(char *, uint8_t qos);
   boolean unsubscribe(char *);
   boolean loop();
   boolean connected();
};


#endif
