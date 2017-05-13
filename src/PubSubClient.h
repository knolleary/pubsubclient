/*
 PubSubClient.h - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#ifndef PubSubClient_h
#define PubSubClient_h

#if defined(ESP8266) || defined(ESP32)
#include <functional>
#include <pgmspace.h>
#endif

#include <Arduino.h>

#include "MQTT.h"

//! Main do-everything class that sketches will use
class PubSubClient {
public:
#ifdef _GLIBCXX_FUNCTIONAL
  typedef std::function<void(const MQTT::Publish&)> callback_t;
#else
  typedef void(*callback_t)(const MQTT::Publish&);
#endif

private:
   IPAddress server_ip;
   String server_hostname;
   uint16_t server_port;
   callback_t _callback;

   Client &_client;
   uint16_t nextMsgId, keepalive;
   uint8_t _max_retries;
   unsigned long lastOutActivity;
   unsigned long lastInActivity;
   bool pingOutstanding;
   bool isSubAckFound;

   //! Receive a message from the client
   /*!
     \return Pointer to message object, nullptr if no message has been received
    */
   MQTT::Message* _recv_message(void);

   //! Send a message and wait for its response message (if it has one)
   /*!
     \param msg The message to send
     \param need_reply Do we wait for the reply message?
    */
   bool _send_message(MQTT::Message& msg, bool need_reply = false);

   //! Process incoming messages
   /*!
     - Calls the callback function when a PUBLISH message comes in
     - Handles the handshake for PUBLISH when qos > 0
     - Handles ping requests and responses
     \param msg Message to process
    */
   void _process_message(MQTT::Message* msg);

   //! Wait for a certain type of packet to come back, optionally check its packet id
   /*!
     Other packets are handed over to _process_message()
     \param wait_type	Message type to match on
     \param wait_pid	Message packet id to match on
     \return True if we received the packet we wanted
    */
   bool _wait_for(MQTT::message_type wait_type, uint16_t wait_pid = 0);

   //! Return the next packet id
   uint16_t _next_packet_id(void) {
     nextMsgId++;
     if (nextMsgId == 0) nextMsgId = 1;
     return nextMsgId;
   }

public:
   //! Simple constructor
   /*!
     Use set_server() before connect()
   */
   PubSubClient(Client& c);

   //! Constructor with the server ip address
   PubSubClient(Client& c, IPAddress &ip, uint16_t port = 1883);
   //! Constructors with the host name
   PubSubClient(Client& c, String hostname, uint16_t port = 1883);

   //! Set the server ip address
   PubSubClient& set_server(IPAddress &ip, uint16_t port = 1883);
   //! Set the server host name
   PubSubClient& set_server(String hostname, uint16_t port = 1883);

   //! Get the callback function
   callback_t callback(void) const { return _callback; }
   //! Set the callback function
   PubSubClient& set_callback(callback_t cb) { _callback = cb; return *this; }
   //! Unset the callback function
   PubSubClient& unset_callback(void) { _callback = nullptr; return * this; }

   //! Set the maximum number of retries when waiting for response packets
   PubSubClient& set_max_retries(uint8_t mr) { _max_retries = mr; return *this; }

   //! Connect to the server with a client id
   /*!
     \param id Client id for this device
    */
   bool connect(String id);

   //! Connect to the server with a client id and "will" parameters
   /*!
     The "will" is a message that is published when this client *unexpectantly*
     disconnects from the broker i.e without sending a DISCONNECT message.
     \param id Client id for this device
     \param willTopic	Topic of the "will" message
     \param willQos	QoS level of the "will" message
     \param willRetain	Retain setting of the "will" message
     \param willMessage	Content (payload) of the "will" message
    */
   bool connect(String id, String willTopic, uint8_t willQos, bool willRetain, String willMessage);

   //! Disconnect from the server
   void disconnect(void);

   //! Publish a string payload
   /*!
     \param topic Topic of the message
     \param payload String text of the message
    */
   bool publish(String topic, String payload);

   //! Publish an arbitrary data payload
   /*!
     \param topic Topic of the message
     \param payload Pointer to contents of the message
     \param plength Length of the message (pointed to by payload) in bytes
     \param retained If true, this message will be stored on the server and
     supplied to any future subscribers whose subscriptions match its topic
     name.
    */
   bool publish(String topic, const uint8_t *payload, uint32_t plength, bool retained = false);

   //! Publish an arbitrary data payload from a callback
   /*!
     \param topic Topic of this message
     \param pcb A callback function that writes the payload directly to the network Client object
     \param length The length of the data that 'pcb' will send
   */
   bool publish(String topic, MQTT::payload_callback_t pcb, uint32_t length, bool retained = false);

   //! Publish an arbitrary data payload stored in "program" memory
   /*!
     \param topic Topic of the message
     \param payload Pointer to contents of the message, stored in "program" memory
     \param plength Length of the message (pointed to by payload) in bytes
     \param retained If true, this message will be stored on the server and
     supplied to any future subscribers whose subscriptions match its topic
     name.
   */
   bool publish_P(String topic, PGM_P payload, uint32_t plength, bool retained = false);

   //! Subscribe to a topic
   /*!
     \param topic Topic filter
     \param qos QoS value. 0 => no handshake, 1 => single handshake, 2 => two-way handshake
    */
   bool subscribe(String topic, uint8_t qos = 0);

   //! Unsubscribe from a topic
   bool unsubscribe(String topic);

   //! Wait for packets to come in, processing them
   /*!
     Also periodically pings the server
   */
   bool loop();

   //! Are we connected?
   bool connected();

   //! Connect with a pre-constructed MQTT message object
   bool connect(MQTT::Connect &conn);
   //! Publish with a pre-constructed MQTT message object
   bool publish(MQTT::Publish &pub);
   //! Subscribe with a pre-constructed MQTT message object
   bool subscribe(MQTT::Subscribe &sub);
   //! Unsubscribe with a pre-constructed MQTT message object
   bool unsubscribe(MQTT::Unsubscribe &unsub);
};


#endif
