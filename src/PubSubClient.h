/*
 PubSubClient.h - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#include <Arduino.h>
//#include <Stream.h>
#include <ESP8266WiFi.h>

#include "MQTT.h"

class PubSubClient {
public:
  typedef void(*callback_t)(String, uint8_t*, unsigned int);

private:
   IPAddress server_ip;
   String server_hostname;
   uint16_t server_port;
   callback_t _callback;
  //   Stream *_stream;

   WiFiClient _client;
   uint16_t nextMsgId;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;

public:
   PubSubClient(IPAddress &ip, uint16_t port = 1883);
   PubSubClient(String hostname, uint16_t port = 1883);

   callback_t callback(void) const { return _callback; }
   PubSubClient& set_callback(callback_t cb);
   PubSubClient& unset_callback(void);

  /*
   Stream* stream(void) const { return _stream; }
   PubSubClient& set_stream(Stream &s);
   PubSubClient& unset_stream(void);
  */

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


   // New methods that take pre-constructed MQTT message objects
   bool connect(MQTT::Connect &conn);
   bool publish(MQTT::Publish &pub);
   bool subscribe(MQTT::Subscribe &sub);
   bool unsubscribe(MQTT::Unsubscribe &unsub);
};


#endif
