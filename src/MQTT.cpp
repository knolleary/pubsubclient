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
  // First some convenience functions
  void write(uint8_t *buf, uint8_t& bufpos, uint16_t data) {
    buf[bufpos++] = data >> 8;
    buf[bufpos++] = data & 0xff;
  }

  void write(uint8_t *buf, uint8_t& bufpos, uint8_t *data, uint8_t dlen) {
    memcpy(buf + bufpos, data, dlen);
    bufpos += dlen;
  }

  void write(uint8_t *buf, uint8_t& bufpos, String str) {
    const char* c = str.c_str();
    uint8_t length_pos = bufpos;
    bufpos += 2;
    uint16_t count = 0;
    while (*c) {
      buf[bufpos++] = *c++;
      count++;
    }
    write(buf, length_pos, count);
  }

  template <typename T>
  T read(uint8_t *buf, uint8_t& pos);

  template <>
  uint8_t read<uint8_t>(uint8_t *buf, uint8_t& pos) {
    return buf[pos++];
  }

  template <>
  uint16_t read<uint16_t>(uint8_t *buf, uint8_t& pos) {
    uint16_t val = buf[pos++] << 8;
    val |= buf[pos++];
    return val;
  }

  template <>
  String read<String>(uint8_t *buf, uint8_t& pos) {
    uint16_t len = read<uint16_t>(buf, pos);
    String val;
    val.reserve(len);
    for (uint8_t i = 0; i < len; i++)
      val += (char)read<uint8_t>(buf, pos);

    return val;
  }


  // Message class
  bool Message::write_fixed_header(uint8_t *buf, uint8_t& bufpos, uint8_t rlength) {
    buf[bufpos] = _type << 4;

    switch (_type) {
    case MQTTPUBLISH:
      buf[bufpos] |= _flags & 0x0f;
      break;
    case MQTTPUBREL:
    case MQTTSUBSCRIBE:
    case MQTTUNSUBSCRIBE:
      buf[bufpos] |= 0x02;
    }
    bufpos++;

    // Remaning length
    do {
      uint8_t digit = rlength & 0x7f;
      rlength >>= 7;
      if (rlength)
	digit |= 0x80;
      buf[bufpos++] = digit;
    } while (rlength);

    return true;
  }

  bool Message::write_packet_id(uint8_t *buf, uint8_t& bufpos) {
    write(buf, bufpos, _packet_id);
  }

  bool Message::send(WiFiClient& wclient) {
    uint8_t packet[MQTT_MAX_PACKET_SIZE];
    uint8_t remaining_length = 0;
    write_variable_header(packet + 5, remaining_length);
    write_payload(packet + 5, remaining_length);

    uint8_t fixed_header[5], fixed_len = 0;
    write_fixed_header(fixed_header, fixed_len, remaining_length);

    uint8_t *real_packet = packet + 5 - fixed_len;
    uint8_t real_len = remaining_length + fixed_len;
    memcpy(real_packet, fixed_header, fixed_len);

    uint8_t sent = wclient.write(const_cast<const uint8_t*>(real_packet), real_len);
    return (sent == real_len);
  }


  uint8_t readByte(WiFiClient& client) {
    while(!client.available()) {}
    return client.read();
  }

  // Parser
  Message* readPacket(WiFiClient& client) {
    // Read type and flags
    uint8_t type = readByte(client);
    uint8_t flags = type & 0x0f;
    type >>= 4;

    // Read the remaining length
    uint8_t lenbuf[4], lenlen = 0;
    uint16_t remaining_length = 0;
    uint8_t shifter = 0;
    uint8_t digit;
    do {
      digit = readByte(client);
      lenbuf[lenlen++] = digit;
      remaining_length += (digit & 0x7f) << shifter;
      shifter += 7;
    } while (digit & 0x80);

    // Read variable header and/or payload
    uint8_t *remaining_data = NULL;
    if (remaining_length > 0) {
      remaining_data = new uint8_t[remaining_length];
      uint16_t r = remaining_length;
      while (client.available() && r) {
	r -= client.read(remaining_data, r);
      }
    }

    // Use the type value to return an object of the appropriate class
    Message *obj;
    switch (type) {
    case MQTTCONNACK:
      obj = new ConnectAck(remaining_data, remaining_length);
      break;

    case MQTTPUBLISH:
      obj = new Publish(flags, remaining_data, remaining_length);
      break;

    case MQTTPUBACK:
      obj = new PublishAck(remaining_data, remaining_length);
      break;

    case MQTTPUBREC:
      obj = new PublishRec(remaining_data, remaining_length);
      break;

    case MQTTPUBREL:
      obj = new PublishRel(remaining_data, remaining_length);
      break;

    case MQTTPUBCOMP:
      obj = new PublishComp(remaining_data, remaining_length);
      break;

    case MQTTSUBACK:
      obj = new SubscribeAck(remaining_data, remaining_length);
      break;

    case MQTTUNSUBACK:
      obj = new UnsubscribeAck(remaining_data, remaining_length);
      break;

    case MQTTPINGREQ:
      obj = new Ping(remaining_data, remaining_length);
      break;

    case MQTTPINGRESP:
      obj = new PingResp(remaining_data, remaining_length);
      break;

    }
    if (remaining_data != NULL)
      delete [] remaining_data;

    return obj;
  }


  // Connect class
  Connect::Connect(String cid) :
    Message(MQTTCONNECT),
    _clean_session(true),
    _clientid(cid),
    _keepalive(MQTT_KEEPALIVE)
  {}

  bool Connect::write_variable_header(uint8_t *buf, uint8_t& bufpos) {
    write(buf, bufpos, "MQTT");	// Protocol name
    buf[bufpos++] = 4;		// Protocol level

    buf[bufpos] = 0;		// Connect flags
    if (_clean_session)
      buf[bufpos] |= 0x02;

    if (_will_topic.length()) {
      buf[bufpos] |= 0x04;

      if (_will_qos > 2)
	_will_qos = 2;
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

  bool Connect::write_payload(uint8_t *buf, uint8_t& bufpos) {
    write(buf, bufpos, _clientid);

    if (_will_topic.length()) {
      write(buf, bufpos, _will_topic);
      write(buf, bufpos, _will_message);
    }

    if (_username.length()) {
      write(buf, bufpos, _username);
      if (_password.length())
	write(buf, bufpos, _password);
    }
  }


  // ConnectAck class
  ConnectAck::ConnectAck(uint8_t* data, uint8_t length) :
    Message(MQTTCONNACK)
  {
    uint8_t pos = 0;
    _session_present = read<uint8_t>(data, pos) & 0x01;
    _rc = read<uint8_t>(data, pos);
  }


  // Publish class
  Publish::Publish(String topic, String payload) :
    Message(MQTTPUBLISH),
    _topic(topic),
    _payload(NULL), _payload_len(0),
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
    Message(MQTTPUBLISH),
    _topic(topic),
    _payload_len(strlen_P((PGM_P)payload)), _payload(new uint8_t[_payload_len + 1]),
    _payload_mine(true)
  {
    strncpy((char*)_payload, (PGM_P)payload, _payload_len);
  }

  Publish Publish_P(String topic, PGM_P payload, uint8_t length) {
    uint8_t *p = new uint8_t[length];
    memcpy_P(p, payload, length);
    return Publish(topic, p, length, true);
  }

  Publish::Publish(uint8_t flags, uint8_t* data, uint8_t length) :
    Message(MQTTPUBLISH, flags),
    _payload(NULL), _payload_len(0),
    _payload_mine(false)
  {
    uint8_t pos = 0;
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

  Publish::~Publish() {
    if (_payload_mine)
      delete [] _payload;
  }

  Publish& Publish::set_qos(uint8_t q, uint16_t pid) {
    if (q > 2)
      q = 2;

    _flags &= ~0x06;
    if (q) {
      _flags |= q << 1;
      _packet_id = pid;
    }
    return *this;
  }

  String Publish::payload_string(void) const {
    String str;
    str.reserve(_payload_len);
    for (uint8_t i = 0; i < _payload_len; i++)
      str += (char)_payload[i];

    return str;
  }

  bool Publish::write_variable_header(uint8_t *buf, uint8_t& bufpos) {
    write(buf, bufpos, _topic);
    if (qos())
      write_packet_id(buf, bufpos);
  }

  bool Publish::write_payload(uint8_t *buf, uint8_t& bufpos) {
    write(buf, bufpos, _payload, _payload_len);
  }

  uint8_t Publish::response_type(void) const {
    switch (qos()) {
    case 0:
      return 0;
    case 1:
      return MQTTPUBACK;
    case 2:
      return MQTTPUBREC;
    }
  }


  // PublishAck class
  PublishAck::PublishAck(uint16_t pid) :
    Message(MQTTPUBACK, pid)
  {}

  PublishAck::PublishAck(uint8_t* data, uint8_t length) :
    Message(MQTTPUBACK)
  {
    uint8_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }


  // PublishRec class
  PublishRec::PublishRec(uint16_t pid) :
    Message(MQTTPUBREC, pid)
  {}

  PublishRec::PublishRec(uint8_t* data, uint8_t length) :
    Message(MQTTPUBREC)
  {
    uint8_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }

  bool PublishRec::write_variable_header(uint8_t *buf, uint8_t& bufpos) {
    write_packet_id(buf, bufpos);
  }


  // PublishRel class
  PublishRel::PublishRel(uint16_t pid) :
    Message(MQTTPUBREL, pid)
  {}

  PublishRel::PublishRel(uint8_t* data, uint8_t length) :
    Message(MQTTPUBREL)
  {
    uint8_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }

  bool PublishRel::write_variable_header(uint8_t *buf, uint8_t& bufpos) {
    write_packet_id(buf, bufpos);
  }


  // PublishComp class
  PublishComp::PublishComp(uint16_t pid) :
    Message(MQTTPUBREC, pid)
  {}

  PublishComp::PublishComp(uint8_t* data, uint8_t length) :
    Message(MQTTPUBCOMP)
  {
    uint8_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }

  bool PublishComp::write_variable_header(uint8_t *buf, uint8_t& bufpos) {
    write_packet_id(buf, bufpos);
  }


  // Subscribe class
  Subscribe::Subscribe(uint16_t pid) :
    Message(MQTTSUBSCRIBE, pid),
    _buffer(NULL), _buflen(0)
  {}

  Subscribe::Subscribe(uint16_t pid, String topic, uint8_t qos) :
    Message(MQTTSUBSCRIBE, pid),
    _buffer(NULL), _buflen(0)
  {
    _buffer = new uint8_t[2 + topic.length() + 1];
    write(_buffer, _buflen, topic);
    _buffer[_buflen++] = qos;
  }

  Subscribe::~Subscribe() {
    delete [] _buffer;
  }

  Subscribe& Subscribe::add_topic(String topic, uint8_t qos) {
    _buffer = (uint8_t*)realloc(_buffer, _buflen + 2 + topic.length() + 1);
    write(_buffer, _buflen, topic);
    _buffer[_buflen++] = qos;
    return *this;
  }

  bool Subscribe::write_variable_header(uint8_t *buf, uint8_t& bufpos) {
    write_packet_id(buf, bufpos);
  }

  bool Subscribe::write_payload(uint8_t *buf, uint8_t& bufpos) {
    write(buf, bufpos, _buffer, _buflen);
  }


  // SubscribeAck class
  SubscribeAck::SubscribeAck(uint8_t* data, uint8_t length) :
    Message(MQTTSUBACK),
    _rcs(NULL)
  {
    uint8_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);

    _num_rcs = length - pos;
    if (_num_rcs > 0) {
      _rcs = new uint8_t[_num_rcs];
      for (uint8_t i = 0; i < _num_rcs; i++)
	_rcs[i] = read<uint8_t>(data, pos);
    }
  }

  SubscribeAck::~SubscribeAck() {
    if (_rcs != NULL)
      delete [] _rcs;
  }


  // Unsubscribe class
  Unsubscribe::Unsubscribe(uint16_t pid) :
    Message(MQTTSUBSCRIBE, pid),
    _buffer(NULL), _buflen(0)
  {}

  Unsubscribe::Unsubscribe(uint16_t pid, String topic) :
    Message(MQTTSUBSCRIBE, pid),
    _buffer(NULL), _buflen(0)
  {
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

  bool Unsubscribe::write_variable_header(uint8_t *buf, uint8_t& bufpos) {
    write_packet_id(buf, bufpos);
  }

  bool Unsubscribe::write_payload(uint8_t *buf, uint8_t& bufpos) {
    write(buf, bufpos, _buffer, _buflen);
  }


  // SubscribeAck class
  UnsubscribeAck::UnsubscribeAck(uint8_t* data, uint8_t length) :
    Message(MQTTUNSUBACK)
  {
    uint8_t pos = 0;
    _packet_id = read<uint16_t>(data, pos);
  }


}; // namespace MQTT
