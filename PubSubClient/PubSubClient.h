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

#define ERR_OK                  0
#define ERR_NOT_CONNECTED       1
#define ERR_TIMEOUT_EXCEEDED    2

// from mqtt-v3r1 
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

class PubSubClient {
private:
   Client _client;
   uint8_t buffer[MAX_PACKET_SIZE];
   uint16_t nextMsgId;
   long lastOutActivity;
   long lastInActivity;
   bool pingOutstanding;
   void (*callback)(char*,uint8_t*,int);
   uint16_t readPacket();
   uint8_t readByte();
   int writeString(char *string, uint16_t str_len);
   int writeRemainingLength(uint16_t length);
public:
   PubSubClient();
   PubSubClient(uint8_t *, uint16_t, void(*)(char*,uint8_t*,int));
   int connect(char *clientId);
   int connect(char *clientId, char *willTopic, uint8_t willQos, uint8_t willRetain, char *willMessage);
   void disconnect();
   int publish(char *topic, char * payload);
   int publish(char *topic, uint8_t *payload, uint16_t payload_length);
   int publish(char *topic, uint8_t *payload, uint16_t payload_length, uint8_t retain);
   int subscribe(char *topic);
   int loop();
   int connected();
};


#endif
