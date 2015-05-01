/*
 PubSubClient.h - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#include <Arduino.h>
#include <Stream.h>
#include <ESP8266WiFi.h>

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

class PubSubClient {
public:
  typedef void(*callback_t)(char*,uint8_t*,unsigned int);

private:
   IPAddress server_ip;
   String server_hostname;
   uint16_t server_port;
   String username, password;
   callback_t _callback;
   Stream *_stream;

   WiFiClient _client;
   uint8_t buffer[MQTT_MAX_PACKET_SIZE];
   uint16_t nextMsgId;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;

   uint16_t readPacket(uint8_t*);
   uint8_t readByte();
   bool write(uint8_t header, uint8_t* buf, uint16_t length);
   uint16_t writeString(String string, uint8_t* buf, uint16_t pos);


public:
   PubSubClient(IPAddress &ip, uint16_t port = 1883);
   PubSubClient(String hostname, uint16_t port = 1883);

   PubSubClient& set_auth(String u, String p);
   PubSubClient& unset_auth(void);

   callback_t callback(void) const { return _callback; }
   PubSubClient& set_callback(callback_t cb);
   PubSubClient& unset_callback(void);

   Stream* stream(void) const { return _stream; }
   PubSubClient& set_stream(Stream &s);
   PubSubClient& unset_stream(void);

   bool connect(String id);
   bool connect(String id, String willTopic, uint8_t willQos, bool willRetain, String willMessage);
   void disconnect(void);
   bool publish(String topic, String payload);
   bool publish(String topic, const uint8_t *payload, unsigned int plength, bool retained = false);
   bool publish_P(String topic, const uint8_t PROGMEM *payload, unsigned int, bool retained = false);
   bool subscribe(String topic, uint8_t qos = 0);
   bool unsubscribe(String topic);

   bool loop();
   bool connected();
};


#endif
