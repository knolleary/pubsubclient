/*
MQTT.cpp - MQTT packet classes
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

#include "MQTT.h"

namespace MQTT {
  //! Write a 16-bit value, big-endian order
  void write(uint8_t *buf, uint32_t& bufpos, uint16_t data) {
    buf[bufpos++] = data >> 8;
    buf[bufpos++] = data & 0xff;
  }

  //! Write an arbitrary chunk of data, with 16-bit length first
  void write(uint8_t *buf, uint32_t& bufpos, uint8_t *data, uint16_t dlen) {
    write(buf, bufpos, dlen);
    memcpy(buf + bufpos, data, dlen);
    bufpos += dlen;
  }

  //! Write a string, with 16-bit length first
  void write(uint8_t *buf, uint32_t& bufpos, String str) {
    const char* c = str.c_str();
    uint32_t length_pos = bufpos;
    bufpos += 2;
    uint16_t count = 0;
    while (*c) {
      buf[bufpos++] = *c++;
      count++;
    }
    write(buf, length_pos, count);
  }

  void write_bare_payload(uint8_t *buf, uint32_t& bufpos, uint8_t *data, uint32_t dlen) {
    memcpy(buf + bufpos, data, dlen);
    bufpos += dlen;
  }

  //! Template function to read from a buffer
  template <typename T>
  T read(uint8_t *buf, uint32_t& pos);

  template <>
  uint8_t read<uint8_t>(uint8_t *buf, uint32_t& pos) {
    return buf[pos++];
  }

  template <>
  uint16_t read<uint16_t>(uint8_t *buf, uint32_t& pos) {
    uint16_t val = buf[pos++] << 8;
    val |= buf[pos++];
    return val;
  }

  template <>
  String read<String>(uint8_t *buf, uint32_t& pos) {
    uint16_t len = read<uint16_t>(buf, pos);
    String val;
    val.reserve(len);
    for (uint16_t i = 0; i < len; i++)
      val += (char)read<uint8_t>(buf, pos);

    return val;
  }

  //! Template function to read from a Client object
  template <typename T>
  T read(Client& client);

  template <>
  uint8_t read<uint8_t>(Client& client) {
    while(!client.available()) {}
    return client.read();
  }

  template <>
  uint16_t read<uint16_t>(Client& client) {
    uint16_t val = read<uint8_t>(client) << 8;
    val |= read<uint8_t>(client);
    return val;
  }

  template <>
  String read<String>(Client& client) {
    uint16_t len = read<uint16_t>(client);
    String val;
    val.reserve(len);
    for (uint16_t i = 0; i < len; i++)
      val += (char)read<uint8_t>(client);

    return val;
  }


  // Message class
  uint8_t Message::fixed_header_length(uint32_t rlength) const {
    if (rlength < 128)
      return 2;
    else if (rlength < 16384)
      return 3;
    else if (rlength < 2097152)
      return 4;
    else
      return 5;
  }

  void Message::write_fixed_header(uint8_t *buf, uint32_t& bufpos, uint32_t rlength) const {
    buf[bufpos] = _type << 4;

    switch (_type) {
    case PUBLISH:
      buf[bufpos] |= _flags & 0x0f;
      break;
    case PUBREL:
    case SUBSCRIBE:
    case UNSUBSCRIBE:
      buf[bufpos] |= 0x02;
    }
    bufpos++;

    // Remaining length
    do {
      uint8_t digit = rlength & 0x7f;
      rlength >>= 7;
      if (rlength)
	digit |= 0x80;
      buf[bufpos++] = digit;
    } while (rlength);
  }

  void Message::write_packet_id(uint8_t *buf, uint32_t& bufpos) const {
    write(buf, bufpos, _packet_id);
  }

  bool Message::send(Client& client) {
    uint32_t variable_header_len = variable_header_length();
    uint32_t remaining_length = variable_header_len + payload_length();
    uint32_t packet_length = fixed_header_length(remaining_length);
    if (_payload_callback == nullptr)
      packet_length += remaining_length;
    else
      packet_length += variable_header_len;

    uint8_t *packet = new uint8_t[packet_length];

    uint32_t pos = 0;
    write_fixed_header(packet, pos, remaining_length);
    write_variable_header(packet, pos);

    write_payload(packet, pos);

    uint32_t sent = client.write(const_cast<const uint8_t*>(packet), packet_length);
    delete [] packet;
    if (sent != packet_length)
      return false;

    if (_payload_callback != nullptr)
      return _payload_callback(client);

    return true;
  }


  // Parser
  Message* readPacket(Client& client) {
    // Read type and flags
    uint8_t type = read<uint8_t>(client);
    uint8_t flags = type & 0x0f;
    type >>= 4;

    // Read the remaining length
    uint32_t remaining_length = 0;
    {
      uint8_t lenbuf[4], lenlen = 0;
      uint8_t shifter = 0;
      uint8_t digit;
      do {
	digit = read<uint8_t>(client);
	lenbuf[lenlen++] = digit;
	remaining_length += (digit & 0x7f) << shifter;
	shifter += 7;
      } while (digit & 0x80);
    }

    // Read variable header and/or payload
    uint8_t *remaining_data = nullptr;
    if (remaining_length > 0) {
      if (remaining_length > MQTT_TOO_BIG) {
	switch (type) {
	case PUBLISH:
	  return new Publish(flags, client, remaining_length);
	case SUBACK:
	  return new SubscribeAck(client, remaining_length);
	default:
	  return nullptr;
	}
      }

      remaining_data = new uint8_t[remaining_length];
      {
	uint8_t *read_point = remaining_data;
	uint32_t rem = remaining_length;
	while (client.available() && rem) {
	  int read_size = client.read(read_point, rem);
	  if (read_size == -1)
	    continue;
	  rem -= read_size;
	  read_point += read_size;
	}
      }
    }

    // Use the type value to return an object of the appropriate class
    Message *obj;
    switch (type) {
    case CONNACK:
      obj = new ConnectAck(remaining_data, remaining_length);
      break;

    case PUBLISH:
      obj = new Publish(flags, remaining_data, remaining_length);
      break;

    case PUBACK:
      obj = new PublishAck(remaining_data, remaining_length);
      break;

    case PUBREC:
      obj = new PublishRec(remaining_data, remaining_length);
      break;

    case PUBREL:
      obj = new PublishRel(remaining_data, remaining_length);
      break;

    case PUBCOMP:
      obj = new PublishComp(remaining_data, remaining_length);
      break;

    case SUBACK:
      obj = new SubscribeAck(remaining_data, remaining_length);
      break;

    case UNSUBACK:
      obj = new UnsubscribeAck(remaining_data, remaining_length);
      break;

    case PINGREQ:
      obj = new Ping;
      break;

    case PINGRESP:
      obj = new PingResp;
      break;

    }
    if (remaining_data != nullptr)
      delete [] remaining_data;

    return obj;
  }


  // Connect class
  Connect::Connect(String cid) :
    Message(CONNECT),
    _clean_session(true),
    _clientid(cid),
    _will_message(nullptr), _will_message_len(0),
    _keepalive(MQTT_KEEPALIVE)
  {}

  Connect& Connect::set_will(String willTopic, String willMessage, uint8_t willQos, bool willRetain) {
    _will_topic = willTopic;
    _will_qos = willQos;
    _will_retain = willRetain;

    if (_will_message != nullptr)
      delete [] _will_message;

    _will_message_len = willMessage.length();
    _will_message = new uint8_t[_will_message_len];
    memcpy(_will_message, willMessage.c_str(), _will_message_len);

    return *this;
  }

  Connect& Connect::set_will(String willTopic, uint8_t *willMessage, uint16_t willMessageLength, uint8_t willQos, bool willRetain) {
    _will_topic = willTopic;
    _will_qos = willQos;
    _will_retain = willRetain;

    if (_will_message != nullptr)
      delete [] _will_message;

    _will_message_len = willMessageLength;
    _will_message = new uint8_t[_will_message_len];
    memcpy(_will_message, willMessage, _will_message_len);

    return *this;
  }

  Connect::~Connect() {
    if (_will_message != nullptr)
      delete [] _will_message;
  }

  uint32_t Connect::variable_header_length(void) const {
    return 10;
  }

  void Connect::write_variable_header(uint8_t *buf, uint32_t& bufpos) const {
    write(buf, bufpos, "MQTT");	// Protocol name
    buf[bufpos++] = 4;		// Protocol level

    buf[bufpos] = 0;		// Connect flags
    if (_clean_session)
      buf[bufpos] |= 0x02;

    if (_will_topic.length()) {
      buf[bufpos] |= 0x04;

      if (_will_qos > 2)
	buf[bufpos] |= 2 << 3;
      else
	buf[bufpos] |= _will_qos << 3;
      buf[bufpos] |= _will_retain << 5;
    }

    if (_username.length()) {
      buf[bufpos] |= 0x80;
      if (_password.length())
	buf[bufpos] |= 0x40;
    }
    bufpos++;

    write(buf, bufpos, _keepalive);	// Keepalive period
  }

  uint32_t Connect::payload_length(void) const {
    uint32_t len = 2 + _clientid.length();
    if (_will_topic.length()) {
      len += 2 + _will_topic.length();
      len += 2 + _will_message_len;
    }
    if (_username.length()) {
      len += 2 + _username.length();
      if (_password.length())
	len += 2 + _password.length();
    }
    return len;
  }

  void Connect::write_payload(uint8_t *buf, uint32_t& bufpos) const {
    write(buf, bufpos, _clientid);

    if (_will_topic.length()) {
      write(buf, bufpos, _will_topic);
      write(buf, bufpos, _will_message, _will_message_len);
    }

    if (_username.length()) {
      write(buf, bufpos, _username);
      if (_password.length())
	write(buf, bufpos, _password);
    }
  }


  // ConnectAck class
  ConnectAck::ConnectAck(uint8_t* data, uint32_t length) :
    Message(CONNACK)
  {
    uint32_t pos = 0;
    uint8_t reserved = read<uint8_t>(data, pos);
    _session_present = reserved & 0x01;
    _rc = read<uint8_t>(data, pos);
  }


  // Publish class
  Publish::Publish(String topic, String payload) :
    Message(PUBLISH),
    _topic(topic),
    _payload(nullptr), _payload_len(0),
    _payload_mine(false)
  {
    if (payload.length() > 0) {
      _payload = new uint8_t[payload.length()];
      memcpy(_payload, payload.c_str(), payload.length());
      _payload_len = payload.length();
      _payload_mine = true;
    }
  }

  Publish::Publish(String topic, const __FlashStringHelper* payload) :
    Message(PUBLISH),
    _topic(topic),
    _payload_len(strlen_P((PGM_P)payload)), _payload(new uint8_t[_payload_len + 1]),
    _payload_mine(true)
  {
    strncpy_P((char*)_payload, (PGM_P)payload, _payload_len);
  }

  Publish Publish_P(String topic, PGM_P payload, uint32_t length) {
    uint8_t *p = new uint8_t[length];
    memcpy_P(p, payload, length);
    return Publish(topic, p, length, true);
  }

  Publish::Publish(uint8_t flags, uint8_t* data, uint32_t length) :
    Message(PUBLISH, flags),
    _payload(nullptr), _payload_len(0),
    _payload_mine(false)
  {
    uint32_t pos = 0;
    _topic = read<String>(data, pos);
    if (qos() > 0)
      _packet_id = read<uint16_t>(data, pos);

    _payload_len = length - pos;
    if (_payload_len > 0) {
      _payload = new uint8_t[_payload_len];
      memcpy(_payload, data + pos, _payload_len);
      _payload_mine = true;
    }
  }

  Publish::Publish(String topic, payload_callback_t pcb, uint32_t length) :
    Message(PUBLISH),
    _topic(topic),
    _payload_len(length),
    _payload(nullptr), _payload_mine(false)
  {
    _payload_callback = pcb;
  }

  Publish::Publish(uint8_t flags, Client& client, uint32_t remaining_length) :
    Message(PUBLISH, flags),
    _payload(nullptr), _payload_len(remaining_length),
    _payload_mine(false)
  {
    _stream_client = &client;

    // Read the topic
    _topic = read<String>(client);
    _payload_len -= 2 + _topic.length();

    if (qos() > 0) {
      // Read the packet id
      _packet_id = read<uint16_t>(client);
      _payload_len -= 2;
    }

    // Client stream is now at the start of the payload
  }

  Publish::~Publish() {
    if ((_payload_mine) && (_payload != nullptr))
      delete [] _payload;
  }

  Publish& Publish::set_qos(uint8_t q) {
    if (q > 2)
      q = 2;

    _flags &= ~0x06;
    if (q) {
      _flags |= q << 1;
      _need_packet_id = true;
    }
    return *this;
  }

  String Publish::payload_string(void) const {
    String str;
    str.reserve(_payload_len);
    for (uint32_t i = 0; i < _payload_len; i++)
      str += (char)_payload[i];

    return str;
  }

  uint32_t Publish::variable_header_length(void) const {
    return 2 + _topic.length() + (qos() ? 2 : 0);
  }

  void Publish::write_variable_header(uint8_t *buf, uint32_t& bufpos) const {
    write(buf, bufpos, _topic);
    if (qos())
      write_packet_id(buf, bufpos);
  }

  uint32_t Publish::payload_length(void) const {
    return _payload_len;
  }

  void Publish::write_payload(uint8_t *buf, uint32_t& bufpos) const {
    if (_payload != nullptr)
      write_bare_payload(buf, bufpos, _payload, _payload_len);
  }

  message_type Publish::response_type(void) const {
    switch (qos()) {
    case 0:
      return None;
    case 1:
      return PUBACK;
    case 2:
      return PUBREC;
    }
  }


  // PublishAck class
  PublishAck::PublishAck(uint16_t pid) :
    Message(PUBACK)
  {
    _packet_id = pid;
  }

  PublishAck::PublishAck(uint8_t* data, uint32_t length) :
    Message(PUBACK)
  {
    uint32_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }


  // PublishRec class
  PublishRec::PublishRec(uint16_t pid) :
    Message(PUBREC)
  {
    _packet_id = pid;
  }

  PublishRec::PublishRec(uint8_t* data, uint32_t length) :
    Message(PUBREC)
  {
    uint32_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }

  uint32_t PublishRec::variable_header_length(void) const {
    return 2;
  }

  void PublishRec::write_variable_header(uint8_t *buf, uint32_t& bufpos) const {
    write_packet_id(buf, bufpos);
  }


  // PublishRel class
  PublishRel::PublishRel(uint16_t pid) :
    Message(PUBREL)
  {
    _packet_id = pid;
  }

  PublishRel::PublishRel(uint8_t* data, uint32_t length) :
    Message(PUBREL)
  {
    uint32_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }

  uint32_t PublishRel::variable_header_length(void) const {
    return 2;
  }

  void PublishRel::write_variable_header(uint8_t *buf, uint32_t& bufpos) const {
    write_packet_id(buf, bufpos);
  }


  // PublishComp class
  PublishComp::PublishComp(uint16_t pid) :
    Message(PUBCOMP)
  {
    _packet_id = pid;
  }

  PublishComp::PublishComp(uint8_t* data, uint32_t length) :
    Message(PUBCOMP)
  {
    uint32_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }

  uint32_t PublishComp::variable_header_length(void) const {
    return 2;
  }

  void PublishComp::write_variable_header(uint8_t *buf, uint32_t& bufpos) const {
    write_packet_id(buf, bufpos);
  }


  // Subscribe class
  Subscribe::Subscribe() :
    Message(SUBSCRIBE),
    _buffer(nullptr), _buflen(0)
  {
    _need_packet_id = true;
  }

  Subscribe::Subscribe(String topic, uint8_t qos) :
    Message(SUBSCRIBE),
    _buffer(nullptr), _buflen(0)
  {
    _need_packet_id = true;
    _buffer = (uint8_t*)malloc(2 + topic.length() + 1);
    write(_buffer, _buflen, topic);
    _buffer[_buflen++] = qos;
  }

  Subscribe::~Subscribe() {
    free(_buffer);
  }

  Subscribe& Subscribe::add_topic(String topic, uint8_t qos) {
    _buffer = (uint8_t*)realloc(_buffer, _buflen + 2 + topic.length() + 1);
    write(_buffer, _buflen, topic);
    _buffer[_buflen++] = qos;
    return *this;
  }

  uint32_t Subscribe::variable_header_length(void) const {
    return 2;
  }

  void Subscribe::write_variable_header(uint8_t *buf, uint32_t& bufpos) const {
    write_packet_id(buf, bufpos);
  }

  uint32_t Subscribe::payload_length(void) const {
    return _buflen;
  }

  void Subscribe::write_payload(uint8_t *buf, uint32_t& bufpos) const {
    if (_buffer != nullptr)
      write_bare_payload(buf, bufpos, _buffer, _buflen);
  }


  // SubscribeAck class
  SubscribeAck::SubscribeAck(uint8_t* data, uint32_t length) :
    Message(SUBACK),
    _rcs(nullptr)
  {
    uint32_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);

    _num_rcs = length - pos;
    if (_num_rcs > 0) {
      _rcs = new uint8_t[_num_rcs];
      for (uint32_t i = 0; i < _num_rcs; i++)
	_rcs[i] = read<uint8_t>(data, pos);
    }
  }

  SubscribeAck::SubscribeAck(Client& client, uint32_t remaining_length) :
    Message(SUBACK),
    _rcs(nullptr),
    _num_rcs(remaining_length - 2)
  {
    _stream_client = &client;

    // Read packet id
    _packet_id = read<uint16_t>(client);

    // Client stream is now at the start of the list of rcs
  }

  SubscribeAck::~SubscribeAck() {
    if (_rcs != nullptr)
      delete [] _rcs;
  }

  uint8_t SubscribeAck::next_rc(void) const {
    return read<uint8_t>(*_stream_client);
  }


  // Unsubscribe class
  Unsubscribe::Unsubscribe() :
    Message(UNSUBSCRIBE),
    _buffer(nullptr), _buflen(0)
  {
    _need_packet_id = true;
  }

  Unsubscribe::Unsubscribe(String topic) :
    Message(UNSUBSCRIBE),
    _buffer(nullptr), _buflen(0)
  {
    _need_packet_id = true;
    _buffer = (uint8_t*)malloc(2 + topic.length());
    write(_buffer, _buflen, topic);
  }

  Unsubscribe::~Unsubscribe() {
    free(_buffer);
  }

  Unsubscribe& Unsubscribe::add_topic(String topic) {
    _buffer = (uint8_t*)realloc(_buffer, _buflen + 2 + topic.length());
    write(_buffer, _buflen, topic);

    return *this;
  }

  uint32_t Unsubscribe::variable_header_length(void) const {
    return 2;
  }

  void Unsubscribe::write_variable_header(uint8_t *buf, uint32_t& bufpos) const {
    write_packet_id(buf, bufpos);
  }

  uint32_t Unsubscribe::payload_length(void) const {
    return _buflen;
  }

  void Unsubscribe::write_payload(uint8_t *buf, uint32_t& bufpos) const {
    if (_buffer != nullptr)
      write_bare_payload(buf, bufpos, _buffer, _buflen);
  }


  // SubscribeAck class
  UnsubscribeAck::UnsubscribeAck(uint8_t* data, uint32_t length) :
    Message(UNSUBACK)
  {
    uint32_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }


} // namespace MQTT
