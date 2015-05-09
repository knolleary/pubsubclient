/*
 PubSubClient.h - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "MQTT.h"

class PubSubClient {
public:
  typedef void(*callback_t)(const MQTT::Publish&);

private:
   IPAddress server_ip;
   String server_hostname;
   uint16_t server_port;
   callback_t _callback;

   WiFiClient _client;
   uint16_t nextMsgId, keepalive;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;

   // Internal function used by wait_for() and loop()
   bool _process_message(MQTT::Message* msg, uint8_t wait_type = 0, uint16_t wait_pid = 0);

   // Wait for a certain type of packet to come back, optionally check its packet id
   bool wait_for(uint8_t wait_type, uint16_t wait_pid = 0);

   bool send_reliably(MQTT::Message* msg);

public:
   PubSubClient(IPAddress &ip, uint16_t port = 1883);
   PubSubClient(String hostname, uint16_t port = 1883);

   callback_t callback(void) const { return _callback; }
   PubSubClient& set_callback(callback_t cb);
   PubSubClient& unset_callback(void);

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

   uint16_t next_packet_id(void) {
     nextMsgId++;
     if (nextMsgId == 0) nextMsgId = 1;
     return nextMsgId;
   }

   // New methods that take pre-constructed MQTT message objects
   bool connect(MQTT::Connect &conn);
   bool publish(MQTT::Publish &pub);
   bool subscribe(MQTT::Subscribe &sub);
   bool unsubscribe(MQTT::Unsubscribe &unsub);
};


#endif
