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
#ifdef ESP8266
#include <pgmspace.h>
#include <functional>
#endif
#include <Client.h>

// MQTT_KEEPALIVE : keepAlive interval in Seconds
#define MQTT_KEEPALIVE 15

class PubSubClient;

//! namespace for classes representing MQTT messages
namespace MQTT {

  enum message_type {
    None,
    CONNECT,		// Client request to connect to Server
    CONNACK,		// Connect Acknowledgment
    PUBLISH,		// Publish message
    PUBACK,		// Publish Acknowledgment
    PUBREC,		// Publish Received (assured delivery part 1)
    PUBREL,		// Publish Release (assured delivery part 2)
    PUBCOMP,		// Publish Complete (assured delivery part 3)
    SUBSCRIBE,		// Client Subscribe request
    SUBACK,		// Subscribe Acknowledgment
    UNSUBSCRIBE,	// Client Unsubscribe request
    UNSUBACK,		// Unsubscribe Acknowledgment
    PINGREQ,		// PING Request
    PINGRESP,		// PING Response
    DISCONNECT,		// Client is Disconnecting
    Reserved,		// Reserved
  };

#ifdef _GLIBCXX_FUNCTIONAL
  typedef std::function<bool(Client&)> payload_callback_t;
#else
  typedef bool(*payload_callback_t)(Client&);
#endif

  //! Abstract base class
  class Message {
  protected:
    message_type _type;
    uint8_t _flags;
    uint16_t _packet_id;	//! Not all message types use a packet id, but most do
    bool _need_packet_id;
    Client* _stream_client;
    payload_callback_t _payload_callback;

    //! Private constructor from type and flags
    Message(message_type t, uint8_t f = 0) :
      _type(t), _flags(f),
      _packet_id(0), _need_packet_id(false),
      _stream_client(NULL)
    {}

    //! Virtual destructor
    virtual ~Message() {}

    //! Length of the fixed header
    /*!
      \param rlength Remaining lengh i.e variable header + payload
    */
    uint8_t fixed_header_length(uint32_t rlength) const;

    //! Write the fixed header to a buffer
    /*!
      \param buf Pointer to start of buffer (never advances)
      \param bufpos Current position in buffer
      \param rlength Remaining lengh i.e variable header + payload
    */
    void write_fixed_header(uint8_t *buf, uint32_t& bufpos, uint32_t rlength) const;

    //! Does this message need a packet id before being sent?
    bool need_packet_id(void) const { return _need_packet_id; }

    //! Set the packet id
    void set_packet_id(uint16_t pid) { _packet_id = pid; }

    //! Write the packet id to a buffer
    /*!
      \param buf Pointer to start of buffer (never advances)
      \param bufpos Current position in buffer
    */
    void write_packet_id(uint8_t *buf, uint32_t& bufpos) const;

    //! Length of variable header
    virtual uint32_t variable_header_length(void) const { return 0; }

    //! Write variable header
    /*!
      \param buf Pointer to start of buffer (never advances)
      \param bufpos Current position in buffer
    */
    virtual void write_variable_header(uint8_t *buf, uint32_t& bufpos) const { }

    //! Length of payload
    virtual uint32_t payload_length(void) const { return 0; }

    //! Write payload
    /*!
      \param buf Pointer to start of buffer (never advances)
      \param bufpos Current position in buffer
    */
    virtual void write_payload(uint8_t *buf, uint32_t& bufpos) const { }

    //! Message type to expect in response to this message
    virtual message_type response_type(void) const { return None; }

    friend PubSubClient;	// Just to allow it to call response_type()

  public:
    //! Send the message out
    bool send(Client& client);

    //! Get the message type
    message_type type(void) const { return _type; }

    //! Get the packet id
    uint16_t packet_id(void) const { return _packet_id; }

    //! Does this message have a network stream for reading the (large) payload?
    bool has_stream(void) const { return _stream_client != NULL; }

  };

  //! Parser
  /*!
    remember to free the object once you're finished with it
  */
  Message* readPacket(Client& client);


  //! Message sent when connecting to a broker
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

    uint32_t variable_header_length(void) const;
    void write_variable_header(uint8_t *buf, uint32_t& bufpos) const;
    uint32_t payload_length(void) const;
    void write_payload(uint8_t *buf, uint32_t& bufpos) const;

    message_type response_type(void) const { return CONNACK; }

  public:
    //! Connect with a client ID
    Connect(String cid);

    //! Set the "clear session" flag
    Connect& set_clean_session(bool cs = true)	{ _clean_session = cs; return *this; }
    //! Unset the "clear session" flag
    Connect& unset_clean_session(void)		{ _clean_session = false; return *this; }

    //! Set the "will" flag and associated attributes
    Connect& set_will(String willTopic, String willMessage, uint8_t willQos = 0, bool willRetain = false) {
      _will_topic = willTopic; _will_message = willMessage; _will_qos = willQos; _will_retain = willRetain;
      return *this;
    }
    //! Unset the "will" flag and associated attributes
    Connect& unset_will(void)			{ _will_topic = ""; return *this; }

    //! Set the username and password for authentication
    Connect& set_auth(String u, String p)	{ _username = u; _password = p; return *this; }
    //! Unset the username and password for authentication
    Connect& unset_auth(void)			{ _username = ""; _password = ""; return *this; }

    //! Get the keepalive period
    uint16_t keepalive(void) const	{ return _keepalive; }
    //! Set the keepalive period
    Connect& set_keepalive(uint16_t k)	{ _keepalive = k; return *this; }

  };


  //! Response to Connect
  class ConnectAck : public Message {
  private:
    bool _session_present;
    uint8_t _rc;

    //! Private constructor from a network buffer
    ConnectAck(uint8_t* data, uint32_t length);

    friend Message* readPacket(Client& client);
  };


  //! Publish a payload to a topic
  class Publish : public Message {
  private:
    String _topic;
    uint8_t *_payload;
    uint32_t _payload_len;
    bool _payload_mine;

    uint32_t variable_header_length(void) const;
    void write_variable_header(uint8_t *buf, uint32_t& bufpos) const;
    uint32_t payload_length(void) const;
    void write_payload(uint8_t *buf, uint32_t& bufpos) const;

    message_type response_type(void) const;

    //! Private constructor from a payload and allowing _payload_mine to be set
    Publish(String topic, uint8_t* payload, uint32_t length, bool mine) :
      Message(PUBLISH),
      _topic(topic),
      _payload(payload), _payload_len(length),
      _payload_mine(mine)
    {}

    //! Private constructor from a network buffer
    Publish(uint8_t flags, uint8_t* data, uint32_t length);

    //! Private constructor from a network stream
    Publish(uint8_t flags, Client& client, uint32_t remaining_length);

    friend Message* readPacket(Client& client);

  public:
    //! Constructor from string payload
    /*!
      \param topic Topic of this message
      \param payload Payload of this message
     */
    Publish(String topic, String payload);

    //! Constructor from arbitrary payload
    /*!
      \param topic Topic of this message
      \param payload Pointer to a block of data
      \param length The length of the data stored at 'payload'
     */
    Publish(String topic, uint8_t* payload, uint32_t length) :
      Publish(topic, payload, length, false)
    {}

    //! Constructor from a callback
    /*!
      \param topic Topic of this message
      \param pcb A callback function that writes the payload directly to the network Client object
      \param length The length of the data that 'pcb' will send
     */
    Publish(String topic, payload_callback_t pcb, uint32_t length);

    //! Constructor from a string stored in flash using the F() macro
    Publish(String topic, const __FlashStringHelper* payload);

    friend Publish Publish_P(String topic, PGM_P payload, uint32_t length);

    ~Publish();

    //! Get retain flag
    bool retain(void) const		{ return _flags & 0x01; }
    //! Set retain flag
    Publish& set_retain(bool r = true)	{ _flags = (_flags & ~0x01) | r; return *this; }
    //! Unset retain flag
    Publish& unset_retain(void)		{ _flags = _flags & ~0x01; return *this; }

    //! Get QoS value
    uint8_t qos(void) const		{ return (_flags >> 1) & 0x03; }
    //! Set QoS value
    Publish& set_qos(uint8_t q);
    //! Unset QoS value
    Publish& unset_qos(void)		{ _flags &= ~0x06; _need_packet_id = false; return *this; }

    //! Get dup flag
    bool dup(void) const		{ return (_flags >> 3) & 0x01; }
    //! Set dup flag
    Publish& set_dup(bool d = true)	{ _flags = (_flags & ~0x08) | (d ? 0x08 : 0); return *this; }
    //! Unset dup flag
    Publish& unset_dup(void)		{ _flags = _flags & ~0x08; return *this; }

    //! Get the topic string
    String topic(void) const { return _topic; }

    //! Get the payload as a string
    String payload_string(void) const;

    //! Get the payload pointer
    uint8_t* payload(void) const { return _payload; }
    //! Get the payload length
    uint32_t payload_len(void) const { return _payload_len; }

    //! Get the network stream for reading the payload
    Client* payload_stream(void) const { return _stream_client; }
  };

  //! A function made to look like a constructor, reading the payload from flash
  Publish Publish_P(String topic, PGM_P payload, uint32_t length);


  //! Response to Publish when qos == 1
  class PublishAck : public Message {
  private:
    //! Private constructor from a network buffer
    PublishAck(uint8_t* data, uint32_t length);

    friend Message* readPacket(Client& client);

  public:
    //! Constructor from a packet id
    PublishAck(uint16_t pid);

  };


  //! First response to Publish when qos == 2
  class PublishRec : public Message {
  private:
    uint32_t variable_header_length(void) const;
    void write_variable_header(uint8_t *buf, uint32_t& bufpos) const;

    message_type response_type(void) const { return PUBREL; }

    //! Private constructor from a network buffer
    PublishRec(uint8_t* data, uint32_t length);

    friend Message* readPacket(Client& client);

  public:
    //! Constructor from a packet id
    PublishRec(uint16_t pid);

  };


  //! Response to PublishRec
  class PublishRel : public Message {
  private:
    uint32_t variable_header_length(void) const;
    void write_variable_header(uint8_t *buf, uint32_t& bufpos) const;

    message_type response_type(void) const { return PUBCOMP; }

    //! Private constructor from a network buffer
    PublishRel(uint8_t* data, uint32_t length);

    friend Message* readPacket(Client& client);

  public:
    //! Constructor from a packet id
    PublishRel(uint16_t pid);

  };


  //! Response to PublishRel
  class PublishComp : public Message {
  private:
    uint32_t variable_header_length(void) const;
    void write_variable_header(uint8_t *buf, uint32_t& bufpos) const;

    //! Private constructor from a network buffer
    PublishComp(uint8_t* data, uint32_t length);

    friend Message* readPacket(Client& client);

  public:
    //! Constructor from a packet id
    PublishComp(uint16_t pid);

  };


  //! Subscribe to one or more topics
  class Subscribe : public Message {
  private:
    uint8_t *_buffer;
    uint32_t _buflen;

    uint32_t variable_header_length(void) const;
    void write_variable_header(uint8_t *buf, uint32_t& bufpos) const;
    uint32_t payload_length(void) const;
    void write_payload(uint8_t *buf, uint32_t& bufpos) const;

    message_type response_type(void) const { return SUBACK; }

  public:
    //! Constructor that starts an empty list of subscriptions
    Subscribe();

    //! Constructor from a topic and optional QoS level
    Subscribe(String topic, uint8_t qos = 0);

    ~Subscribe();

    //! Add another topic and optional QoS level
    Subscribe& add_topic(String topic, uint8_t qos = 0);

  };


  //! Response to Subscribe
  class SubscribeAck : public Message {
  private:
    uint8_t *_rcs;
    uint32_t _num_rcs;

    //! Private constructor from a network buffer
    SubscribeAck(uint8_t* data, uint32_t length);

    //! Private constructor from a network stream
    SubscribeAck(Client& client, uint32_t remaining_length);

    friend Message* readPacket(Client& client);

  public:
    ~SubscribeAck();

    //! Get the number of return codes available
    uint32_t num_rcs(void) const { return _num_rcs; }

    //! Get a return code
    uint8_t rc(uint8_t i) const { return _rcs[i]; }

    //! Get the next return code from a stream
    uint8_t next_rc(void) const;

  };


  //! Unsubscribe from one or more topics
  class Unsubscribe : public Message {
  private:
    uint8_t *_buffer;
    uint32_t _buflen;

    uint32_t variable_header_length(void) const;
    void write_variable_header(uint8_t *buf, uint32_t& bufpos) const;
    uint32_t payload_length(void) const;
    void write_payload(uint8_t *buf, uint32_t& bufpos) const;

    message_type response_type(void) const { return UNSUBACK; }

  public:
    //! Constructor that starts with an empty list of unsubscriptions
    Unsubscribe();

    //! Constructor from a topic
    Unsubscribe(String topic);

    ~Unsubscribe();

    //! Add another topic to unsubscribe from
    Unsubscribe& add_topic(String topic);

  };


  //! Response to Unsubscribe
  class UnsubscribeAck : public Message {
  private:
    //! Private constructor from a network buffer
    UnsubscribeAck(uint8_t* data, uint32_t length);

    friend Message* readPacket(Client& client);

  };


  //! Ping the broker
  class Ping : public Message {
  private:
    message_type response_type(void) const { return PINGRESP; }

  public:
    //! Constructor
    Ping() :
      Message(PINGREQ)
    {}

  };


  //! Response to Ping
  class PingResp : public Message {
  public:
    //! Constructor
    PingResp() :
      Message(PINGRESP)
    {}

  };


  //! Disconnect from the broker
  class Disconnect : public Message {
  public:
    //! Constructor
    Disconnect() :
      Message(DISCONNECT)
    {}

  };

}

