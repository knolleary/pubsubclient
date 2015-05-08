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
#include <ESP8266WiFi.h>

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

namespace MQTT {
  // Abstract base class
  class Message {
  protected:
    uint8_t _type, _flags;

    Message(uint8_t t, uint8_t f = 0) :
      _type(t), _flags(f)
    {}

    // Write the fixed header to a buffer
    virtual bool write_fixed_header(uint8_t *buf, uint8_t& len, uint8_t rlength);

    // Abstract methods to be implemented by derived classes
    virtual bool write_variable_header(uint8_t *buf, uint8_t& len) = 0;
    virtual bool write_payload(uint8_t *buf, uint8_t& len) {}

  public:
    // Send the message out
    bool send(WiFiClient& wclient);

    // Get message type
    uint8_t type(void) const { return _type; }
  };

  // Parser
  // remember to free the object once you're finished with it
  Message* readPacket(WiFiClient &client);


  // Role class
  class with_packet_id {
  protected:
    uint16_t _packet_id;

    with_packet_id(uint16_t pid) :
      _packet_id(pid)
    {}

    bool write_packet_id(uint8_t *buf, uint8_t& len);

  public:
    uint16_t packet_id(void) const { return _packet_id; }
  };


  // Abstract role class
  class with_payload {
  private:
    virtual bool write_payload(uint8_t *buf, uint8_t& len) = 0;
  public:
  };


  // Message sent when connecting to a broker
  class Connect : public Message, public with_payload {
  private:
    bool _clean_session;
    uint8_t _will_qos;
    bool _will_retain;

    String _clientid;
    String _will_topic;
    String _will_message;
    String _username, _password;

    uint16_t _keepalive;

    bool write_variable_header(uint8_t *buf, uint8_t& len);
    bool write_payload(uint8_t *buf, uint8_t& len);

  public:
    // Connect with a client ID
    Connect(String cid);

    // Set or unset the "clear session" flag
    Connect& set_clean_session(bool cs = true);
    Connect& unset_clean_session(void);

    // Set or unset the "will" flag and associated attributes
    Connect& set_will(String willTopic, String willMessage, uint8_t willQos = 0, bool willRetain = false);
    Connect& unset_will(void);

    // Set or unset the username and password for authentication
    Connect& set_auth(String u, String p);
    Connect& unset_auth(void);

    uint16_t keepalive(void) const { return _keepalive; }
    // Set the keepalive period
    Connect& set_keepalive(uint16_t k);
  };


  // Response to Connect
  class ConnectAck : public Message {
  private:
    bool _session_present;
    uint8_t _rc;

    bool write_variable_header(uint8_t *buf, uint8_t& len) {}

  public:
    ConnectAck(uint8_t* data, uint8_t length);
  };


  // Publish a payload to a topic
  class Publish : public Message, public with_packet_id, public with_payload {
  private:
    String _topic;
    uint8_t *_payload, _payload_len;
    bool _payload_mine;

    bool write_variable_header(uint8_t *buf, uint8_t& len);
    bool write_payload(uint8_t *buf, uint8_t& len);

  public:
    // Constructors for creating our own message to publish
    Publish(String topic, String payload);
    Publish(String topic, uint8_t* payload, uint8_t length);

    // Constructor for incoming messages
    Publish(uint8_t flags, uint8_t* data, uint8_t length);

    ~Publish();

    bool retain(void) const { return _flags & 0x01; }
    Publish& set_retain(bool r = true);
    Publish& unset_retain(void);

    uint8_t qos(void) const { return (_flags >> 1) & 0x03; }
    Publish& set_qos(uint8_t q, uint16_t pid = 0);
    Publish& unset_qos(void);

    bool dup(void) const { return (_flags >> 3) & 0x01; }
    Publish& set_dup(bool d = true);
    Publish& unset_dup(void);

    String topic(void) const { return _topic; }

    String payload_string(void) const;
    uint8_t* payload(void) const { return _payload; }
    uint8_t payload_len(void) const { return _payload_len; }
  };


  // Response to Publish when qos == 0
  class PublishAck : public Message, public with_packet_id {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& len) {}

  public:
    PublishAck(uint16_t pid);

    PublishAck(uint8_t* data, uint8_t length);
  };


  // First response to Publish when qos > 0
  class PublishRec : public Message, public with_packet_id {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& len);

  public:
    PublishRec(uint16_t pid);

    PublishRec(uint8_t* data, uint8_t length);
  };


  // Response to PublishRec
  class PublishRel : public Message, public with_packet_id {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& len);

  public:
    PublishRel(uint16_t pid);

    PublishRel(uint8_t* data, uint8_t length);
  };


  // Response to PublishRec
  class PublishComp : public Message, public with_packet_id {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& len);

  public:
    PublishComp(uint16_t pid);

    PublishComp(uint8_t* data, uint8_t length);
  };


  // Subscribe to one or more topics
  class Subscribe : public Message, public with_packet_id, public with_payload {
  private:
    uint8_t *_buffer, _buflen;

    bool write_variable_header(uint8_t *buf, uint8_t& len);
    bool write_payload(uint8_t *buf, uint8_t& len);

  public:
    // Subscribe with a packet id, topic, and optional QoS level
    Subscribe(uint16_t pid, String topic, uint8_t qos = 0);

    ~Subscribe();

    // Add another topic and optional QoS level
    Subscribe& add_topic(String topic, uint8_t qos = 0);
  };


  // Response to Subscribe
  class SubscribeAck : public Message, public with_packet_id {
  private:
    uint8_t *_rcs, _num_rcs;

    bool write_variable_header(uint8_t *buf, uint8_t& len) {}

  public:
    SubscribeAck(uint8_t* data, uint8_t length);
    ~SubscribeAck();

    uint8_t num_rcs(void) const { return _num_rcs; }
    uint8_t rc(uint8_t i) const { return _rcs[i]; }
  };


  // Unsubscribe from one or more topics
  class Unsubscribe : public Message, public with_packet_id, public with_payload {
  private:
    uint8_t *_buffer, _buflen;

    bool write_variable_header(uint8_t *buf, uint8_t& len);
    bool write_payload(uint8_t *buf, uint8_t& len);

  public:
    // Unsubscribe from a topic, with a packet id
    Unsubscribe(uint16_t pid, String topic);

    ~Unsubscribe();

    // Add another topic to unsubscribe from
    Unsubscribe& add_topic(String topic);
  };


  // Response to Unsubscribe
  class UnsubscribeAck : public Message, public with_packet_id {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& len) {}

  public:
    UnsubscribeAck(uint8_t* data, uint8_t length);
  };


  // Ping the broker
  class Ping : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& len) {}

  public:
    Ping() :
      Message(MQTTPINGREQ)
    {}

    Ping(uint8_t* data, uint8_t length) :
      Message(MQTTPINGREQ)
    {}
  };


  // Response to Ping
  class PingResp : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& len) {}

  public:
    PingResp() :
      Message(MQTTPINGRESP)
    {}

    PingResp(uint8_t* data, uint8_t length) :
      Message(MQTTPINGRESP)
    {}
  };


  // Disconnect from the broker
  class Disconnect : public Message {
  private:
    bool write_variable_header(uint8_t *buf, uint8_t& len) {}

  public:
    Disconnect() :
      Message(MQTTDISCONNECT)
    {}
  };

};

