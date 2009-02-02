/*
 PubSubClient.h - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#include "Client.h"

#define MAX_PACKET_SIZE 128
#define KEEPALIVE 15000 // max value = 255000


class PubSubClient {
private:
   Client _client;
   uint8_t buffer[MAX_PACKET_SIZE];
   uint8_t nextMsgId;
   long lastActivity;
   void (*callback)(char*,uint8_t*,int);
   uint8_t readPacket();
   int write(uint8_t header, uint8_t* buf, uint8_t length);
   uint8_t writeString(char* string, uint8_t* buf, uint8_t pos);
public:
   PubSubClient(uint8_t *, uint16_t, void(*)(char*,uint8_t*,int));
   int connect(char *);
   int connect(char*, char*, uint8_t, uint8_t, char*);
   void disconnect();
   int publish(char *, char *);
   int publish(char *, uint8_t *, uint8_t);
   void subscribe(char *);
   int loop();
   int connected();
};


#endif
