/*
MQTT.h - MQTT packet classes
Copyright (C) 2015 Ian Tester

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <stdint.h>
#include <pgmspace.h>
#include <WiFiClient.h>

// MQTT_MAX_PACKET_SIZE : Maximum packet size
#define MQTT_MAX_PACKET_SIZE 128

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#define MQTT_KEEPALIVE 15

#define MQTTCONNECT     1  // Client request to connect to Server
#define MQTTCONNACK     2  // Connect Acknowledgment
#define MQTTPUBLISH     3  // Publish message
#define MQTTPUBACK      4  // Publish Acknowledgment
#define MQTTPUBREC      5  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8  // Client Subscribe request
#define MQTTSUBACK      9  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 // Client Unsubscribe request
#define MQTTUNSUBACK    11 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 // PING Request
#define MQTTPINGRESP    13 // PING Response
#define MQTTDISCONNECT  14 // Client is Disconnecting
#define MQTTReserved    15 // Reserved

class PubSubClient;

namespace MQTT {
  // Abstract base class
  class Message {
  protected:
    uint8_t _type, _flags;
    uint16_t _packet_id;	// Not all message types use a packet id, but most do

    Message(uint8_t t, uint8_t f = 0) :
      _type(t), _flags(f),
      _packet_id(0)
    {}

    Message(uint8_t t, uint16_t pid) :
      _type(t), _flags(0),
      _packet_id(pid)
    {}

    virtual ~Message() {}

    // Write the fixed header to a buffer
    bool write_fixed_header(uint8_t *buf, uint8_t& bufpos, uint8_t rlength);

    bool write_packet_id(uint8_t *buf, uint8_t& bufpos);

    // Abstract methods to be implemented by derived classes
    virtual bool write_variable_header(uint8_t *buf, uint8_t& bufpos) = 0;
    virtual bool write_payload(uint8_t *buf, uint8_t& bufpos) {}

    virtual uint8_t response_type(void) const { return 0; }

    friend PubSubClient;	// Just to allow it to call response_type()

  public:
    // Send the message out
    bool send(WiFiClient& wclient);

    // Get the message type
    uint8_t type(void) const { return _type; }

    // Get the packet id
    uint16_t packet_id(void) const { return _packet_id; }
  };

  // Parser
  // remember to free the object once you're finished with it
  Message* readPacket(WiFiClient &client);


  // Message sent when connecting to a broker
  class Connect : public Message {
  private:
    bool _clean_session;
    uint8_t _will_qos;
    bool _will_retain;

    String _clientid;
    String _will_topic;
    String _will_message;
    String _username, _password;

    uint16_t _keepalive;

    bool write_variable_header(uint8_t *buf, uint8_t& bufpos);
    bool write_payload(uint8_t *buf, uint8_t& bufpos);

    uint8_t response_type(void) const { return MQTTCONNACK; }

  public:
    // Connect with a client ID
    Connect(String cid);

    // Set or unset the "clear session" flag
    Connect& set_clean_session(bool cs = true)	{ _clean_session = cs; return *this; }
    Connect& unset_clean_session(void)		{ _clean_session = false; return *this; }

    // Set or unset the "will" flag and associated attributes
    Connect& set_will(String willTopic, String willMessage, uint8_t willQos = 0, bool willRetain = false) {
      _will_topic = willTopic; _will_message = willMessage; _will_qos = willQos; _will_retain = willRetain;
      return *this;
    }
    Connect& unset_will(void)			{ _will_topic = ""; return *this; }

    // Set or unset the username and password for authentication
    Connect& set_auth(String u, String p)	{ _username = u; _password = p; return *this; }
    Connect& unset_auth(void)			{ _username = ""; _password = ""; return *this; }

    // Get or set the keepalive period
    uint16_t keepalive(void) const	{ return _keepalive; }
    Connect& set_keepalive(uint16_t k)	{ _keepalive = k; return *this; }

  };


  // Response to Connect
  class ConnectAck : public Message {
  private:
    bool _session_present;
    uint8_t _rc;

    bool write_variable_header(uint8_t *buf, uint8_t& bufpos) {}

  public:
    // Construct from a network buffer
    ConnectAck(uint8_t* data, uint8_t length);
  };


  // Publish a payload to a topic
  class Publish : public Message {
  private:
    String _topic;
    uint8_t *_payload, _payload_len;
    bool _payload_mine;

    bool write_variable_header(uint8_t *buf, uint8_t& bufpos);
    bool write_payload(uint8_t *buf, uint8_t& bufpos);

    uint8_t response_type(void) const;

    Publish(String topic, uint8_t* payload, uint8_t length, bool mine) :
      Message(MQTTPUBLISH),
      _topic(topic),
      _payload(payload), _payload_len(length),
      _payload_mine(mine)
    {}

  public:
    // Constructors for creating our own message to publish
    Publish(String topic, String payload);
    Publish(String topic, uint8_t* payload, uint8_t length) :
      Publish(topic, payload, length, false)
    {}

    // Constructor using a string stored in flash using the F() macro
    Publish(String topic, const __FlashStringHelper* payload);
    // A function made to look like a constructor, reading the payload from flash
    friend Publish Publish_P(String topic, PGM_P payload, uint8_t length);

    // Construct from a network buffer
    Publish(uint8_t flags, uint8_t* data, uint8_t length);

    ~Publish();

    // Get or set retain flag
    bool retain(void) const		{ return _flags & 0x01; }
    Publish& set_retain(bool r = true)	{ _flags = (_flags & ~0x01) | r; return *this; }
    Publish& unset_retain(void)		{ _flags = _flags & ~0x01; return *this; }

    // Get or set QoS value
    uint8_t qos(void) const		{ return (_flags >> 1) & 0x03; }
    Publish& set_qos(uint8_t q, uint16_t pid = 0);
    Publish& unset_qos(void)		{ _flags &= ~0x06; return *this; }

    // Get or set dup flag
    bool dup(void) const		{ return (_flags >> 3) & 0x01; }
    Publish& set_dup(bool d = true)	{ _flags = (_flags & ~0x08) | (d ? 0x08 : 0); return *this; }
    Publish& unset_dup(void)		{ _flags = _flags & ~0x08; return *this; }

    // Stopic string
    String topic(void) const { return _topic; }

    // Payload as a string
    String payload_string(void) const;

    // Get the payload pointer and length
    uint8_t* payload(void) const { return _payload; }
    uint8_t payload_len(void) const { return _payload_len; }

  };

  Publish Publish_P(String topic, PGM_P payload, uint8_t length);


  // Response to Publish when qos == 1
  class PublishAck : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& bufpos) {}

  public:
    // Construct with a packet id
    PublishAck(uint16_t pid);

    // Construct from a network buffer
    PublishAck(uint8_t* data, uint8_t length);
  };


  // First response to Publish when qos == 2
  class PublishRec : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& bufpos);

    uint8_t response_type(void) const { return MQTTPUBREL; }

  public:
    // Construct with a packet id
    PublishRec(uint16_t pid);

    // Construct from a network buffer
    PublishRec(uint8_t* data, uint8_t length);

  };


  // Response to PublishRec
  class PublishRel : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& bufpos);

    uint8_t response_type(void) const { return MQTTPUBCOMP; }

  public:
    // Construct with a packet id
    PublishRel(uint16_t pid);

    // Construct from a network buffer
    PublishRel(uint8_t* data, uint8_t length);

  };


  // Response to PublishRel
  class PublishComp : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& bufpos);

  public:
    // Construct with a packet id
    PublishComp(uint16_t pid);

    // Construct from a network buffer
    PublishComp(uint8_t* data, uint8_t length);
  };


  // Subscribe to one or more topics
  class Subscribe : public Message {
  private:
    uint8_t *_buffer, _buflen;

    bool write_variable_header(uint8_t *buf, uint8_t& bufpos);
    bool write_payload(uint8_t *buf, uint8_t& bufpos);

    uint8_t response_type(void) const { return MQTTSUBACK; }

  public:
    // Constructor that starts with a packet id and empty list of subscriptions
    Subscribe(uint16_t pid);

    // Subscribe with a packet id, topic, and optional QoS level
    Subscribe(uint16_t pid, String topic, uint8_t qos = 0);

    ~Subscribe();

    // Add another topic and optional QoS level
    Subscribe& add_topic(String topic, uint8_t qos = 0);

  };


  // Response to Subscribe
  class SubscribeAck : public Message {
  private:
    uint8_t *_rcs, _num_rcs;

    bool write_variable_header(uint8_t *buf, uint8_t& bufpos) {}

  public:
    // Construct from a network buffer
    SubscribeAck(uint8_t* data, uint8_t length);

    ~SubscribeAck();

    // Get the number of return codes available
    uint8_t num_rcs(void) const { return _num_rcs; }

    // Get a return code
    uint8_t rc(uint8_t i) const { return _rcs[i]; }

  };


  // Unsubscribe from one or more topics
  class Unsubscribe : public Message {
  private:
    uint8_t *_buffer, _buflen;

    bool write_variable_header(uint8_t *buf, uint8_t& bufpos);
    bool write_payload(uint8_t *buf, uint8_t& bufpos);

    uint8_t response_type(void) const { return MQTTUNSUBACK; }

  public:
    // Constructor that starts with a packet id and empty list of unsubscriptions
    Unsubscribe(uint16_t pid);

    // Unsubscribe from a topic, with a packet id
    Unsubscribe(uint16_t pid, String topic);

    ~Unsubscribe();

    // Add another topic to unsubscribe from
    Unsubscribe& add_topic(String topic);

  };


  // Response to Unsubscribe
  class UnsubscribeAck : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& bufpos) {}

  public:
    // Construct from a network buffer
    UnsubscribeAck(uint8_t* data, uint8_t length);

  };


  // Ping the broker
  class Ping : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& bufpos) {}

    uint8_t response_type(void) const { return MQTTPINGRESP; }

  public:
    // Constructor
    Ping() :
      Message(MQTTPINGREQ)
    {}

    // Construct from a network buffer
    Ping(uint8_t* data, uint8_t length) :
      Message(MQTTPINGREQ)
    {}

  };


  // Response to Ping
  class PingResp : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& bufpos) {}

  public:
    // Constructor
    PingResp() :
      Message(MQTTPINGRESP)
    {}

    // Construct from a network buffer
    PingResp(uint8_t* data, uint8_t length) :
      Message(MQTTPINGRESP)
    {}

  };


  // Disconnect from the broker
  class Disconnect : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& bufpos) {}

  public:
    // Constructor
    Disconnect() :
      Message(MQTTDISCONNECT)
    {}

  };

};

