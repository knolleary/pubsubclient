/*
 PubSubClient.h - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#include <Arduino.h>
#include <pgmspace.h>

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
   uint8_t _max_retries;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;

   // Internal function used by wait_for() and loop()
   bool _process_message(MQTT::Message* msg, uint8_t wait_type = 0, uint16_t wait_pid = 0);

   // Wait for a certain type of packet to come back, optionally check its packet id
   bool wait_for(uint8_t wait_type, uint16_t wait_pid = 0);

   bool send_reliably(MQTT::Message* msg);

public:
   // Empty constructor - use set_server() later, before connect()
   PubSubClient();

   // Constructors with the server ip address or host name
   PubSubClient(IPAddress &ip, uint16_t port = 1883);
   PubSubClient(String hostname, uint16_t port = 1883);

   // Set the server ip address or host name
   PubSubClient& set_server(IPAddress &ip, uint16_t port = 1883);
   PubSubClient& set_server(String hostname, uint16_t port = 1883);

   // Get or set the callback function
   callback_t callback(void) const { return _callback; }
   PubSubClient& set_callback(callback_t cb) { _callback = cb; return *this; }
   PubSubClient& unset_callback(void) { _callback = NULL; return * this; }

   // Set the maximum number of retries when waiting for response packets
   PubSubClient& set_max_retries(uint8_t mr) { _max_retries = mr; return *this; }

   // Connect to the server with a client id
   bool connect(String id);

   // Connect to the server with a client id and "will" parameters
   bool connect(String id, String willTopic, uint8_t willQos, bool willRetain, String willMessage);

   // Disconnect from the server
   void disconnect(void);

   // Publish a string payload
   bool publish(String topic, String payload);

   // Publish an arbitrary data payload
   bool publish(String topic, const uint8_t *payload, unsigned int plength, bool retained = false);

   // Publish an arbitrary data payload stored in "program memory"
   bool publish_P(String topic, PGM_P payload, unsigned int, bool retained = false);

   // Subscribe to a topic
   bool subscribe(String topic, uint8_t qos = 0);

   // Unsubscribe from a topic
   bool unsubscribe(String topic);

   // Wait for packets to come in, processing them
   // Also periodically pings the server
   bool loop();

   // Are we connected?
   bool connected();

   // Return the next packet id
   // Needed for constructing our own publish (with QoS>0) or (un)subscribe messages
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
