/*
  PubSubClient.cpp - A simple client for MQTT.
  Nick O'Leary
  http://knolleary.net
*/

#include <stdio.h>

#include "PubSubClient.h"
#include "Arduino.h"

PubSubClient::PubSubClient() {
    this->_state = MQTT_DISCONNECTED;
    this->_client = NULL;
    this->stream = NULL;
    this->setCallback(NULL);
}

PubSubClient::PubSubClient(Client& client) {
    this->_state = MQTT_DISCONNECTED;
    this->setClient(client);
    this->stream = NULL;
}

PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Client& client) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(addr, port);
    this->setClient(client);
    this->stream = NULL;
}
PubSubClient::PubSubClient(IPAddress addr, uint16_t port, Client& client, Stream& stream) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(addr, port);
    this->setClient(client);
    this->setStream(stream);
}
PubSubClient::PubSubClient(IPAddress addr, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(addr, port);
    this->setCallback(callback);
    this->setClient(client);
    this->stream = NULL;
}
PubSubClient::PubSubClient(IPAddress addr, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client, Stream& stream) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(addr,port);
    this->setCallback(callback);
    this->setClient(client);
    this->setStream(stream);
}

PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, Client& client) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(ip, port);
    this->setClient(client);
    this->stream = NULL;
}
PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, Client& client, Stream& stream) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(ip,port);
    this->setClient(client);
    this->setStream(stream);
}
PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(ip, port);
    this->setCallback(callback);
    this->setClient(client);
    this->stream = NULL;
}
PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client, Stream& stream) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(ip,port);
    this->setCallback(callback);
    this->setClient(client);
    this->setStream(stream);
}

PubSubClient::PubSubClient(const char* domain, uint16_t port, Client& client) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(domain,port);
    this->setClient(client);
    this->stream = NULL;
}
PubSubClient::PubSubClient(const char* domain, uint16_t port, Client& client, Stream& stream) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(domain,port);
    this->setClient(client);
    this->setStream(stream);
}
PubSubClient::PubSubClient(const char* domain, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(domain,port);
    this->setCallback(callback);
    this->setClient(client);
    this->stream = NULL;
}
PubSubClient::PubSubClient(const char* domain, uint16_t port, MQTT_CALLBACK_SIGNATURE, Client& client, Stream& stream) {
    this->_state = MQTT_DISCONNECTED;
    this->setServer(domain,port);
    this->setCallback(callback);
    this->setClient(client);
    this->setStream(stream);
}

boolean PubSubClient::connect(const char *id) {
    return this->connect(id, NULL, NULL, 0, 0, 0, 0, 1);
}

boolean PubSubClient::connect(const char *id, const char *user, const char *pass) {
    return this->connect(id, user, pass, 0, 0, 0, 0, 1);
}

boolean PubSubClient::connect(const char *id, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage) {
    return this->connect(id, NULL, NULL, willTopic, willQos, willRetain, willMessage, 1);
}

boolean PubSubClient::connect(const char *id, const char *user, const char *pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage) {
    return this->connect(id, user, pass, willTopic, willQos, willRetain, willMessage, 1);
}

boolean PubSubClient::connect(const char *id, const char *user, const char *pass, const char* willTopic, uint8_t willQos, boolean willRetain, const char* willMessage, boolean cleanSession) {
  if (!this->connected()) {
    int result = 0;

    if (this->_client->connected()) result = 1;
    else if (NULL != domain) result = this->_client->connect(this->domain, this->port);
    else result = this->_client->connect(this->ip, this->port);

    if (1 == result) {
      this->nextMsgId = 1;

      // Leave room in the buffer for header and variable length field
      uint16_t length = MQTT_MAX_HEADER_SIZE;
      unsigned int j;

#if MQTT_VERSION == MQTT_VERSION_3_1
      uint8_t d[9] = {0x00, 0x06, 'M' ,'Q' ,'I' ,'s' ,'d' ,'p' , MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 9
#elif MQTT_VERSION == MQTT_VERSION_3_1_1
      uint8_t d[7] = {0x00, 0x04, 'M', 'Q', 'T', 'T', MQTT_VERSION};
#define MQTT_HEADER_VERSION_LENGTH 7
#endif
      for (j = 0; j < MQTT_HEADER_VERSION_LENGTH; j++) this->buffer[length++] = d[j];

      uint8_t v = willTopic ? 0x04 | (willQos << 3) | (willRetain << 5) : 0x00;
      if (cleanSession) v = v | 0x02;

      if (user != NULL) {
          v = v|0x80;
          if (pass != NULL) v = v|(0x80>>1);
      }

      this->buffer[length++] = v;
      this->buffer[length++] = ((MQTT_KEEPALIVE) >> 8);
      this->buffer[length++] = ((MQTT_KEEPALIVE) & 0xFF);

      CHECK_STRING_LENGTH(length, id)
      length = this->writeString(id, this->buffer, length);
      if (willTopic) {
        CHECK_STRING_LENGTH(length, willTopic)
        length = this->writeString(willTopic, this->buffer, length);
        CHECK_STRING_LENGTH(length, willMessage)
        length = this->writeString(willMessage, this->buffer, length);
      }

      if (user != NULL) {
        CHECK_STRING_LENGTH(length, user)
        length = this->writeString(user, this->buffer, length);
        if (pass != NULL) {
            CHECK_STRING_LENGTH(length, pass)
            length = this->writeString(pass, this->buffer, length);
        }
      }

      this->write(MQTTCONNECT, this->buffer, length - MQTT_MAX_HEADER_SIZE);

      this->lastInActivity = this->lastOutActivity = ::millis();

      while (!this->_client->available()) {
        unsigned long t = ::millis();
        if (t - this->lastInActivity >= ((int32_t) MQTT_SOCKET_TIMEOUT * 1000UL)) {
          this->_state = MQTT_CONNECTION_TIMEOUT;
          this->_client->stop();
          return false;
        }
      }
      uint8_t llen;
      uint16_t len = this->readPacket(&llen);

      if (len == 4) {
        if (this->buffer[3] == 0) {
            this->lastInActivity = ::millis();
            this->pingOutstanding = false;
            this->_state = MQTT_CONNECTED;
            return true;
        } else this->_state = this->buffer[3];
      }
      this->_client->stop();
    } else this->_state = MQTT_CONNECT_FAILED;

    return false;
  }
  return true;
}

// reads a byte into result
boolean PubSubClient::readByte(uint8_t* result) {
   uint32_t previousMillis = millis();
   while (!this->_client->available()) {
     ::yield();
     uint32_t currentMillis = ::millis();
     if (currentMillis - previousMillis >= ((int32_t) MQTT_SOCKET_TIMEOUT * 1000)) return false;
   }
   *result = this->_client->read();

   return true;
}

// reads a byte into result[*index] and increments index
boolean PubSubClient::readByte(uint8_t* result, uint16_t* index) {
  uint16_t current_index = *index;
  uint8_t * write_address = &(result[current_index]);
  if (this->readByte(write_address)) {
    *index = current_index + 1;
    return true;
  }
  return false;
}

uint16_t PubSubClient::readPacket(uint8_t* lengthLength) {
  uint16_t len = 0;
  if (!this->readByte(this->buffer, &len)) return 0;

  bool isPublish = (this->buffer[0] & 0xF0) == MQTTPUBLISH;
  uint32_t multiplier = 1;
  uint16_t length = 0;
  uint8_t digit = 0;
  uint16_t skip = 0;
  uint8_t start = 0;

  do {
    if (len == 5) {
      ::sprintf(this->_statusMessage, "Disconnecting due to short publish message: %d", len);

      // Invalid remaining length encoding - kill the connection
      this->_state = MQTT_DISCONNECTED;
      this->_client->stop();
      return 0;
    }
    if(!this->readByte(&digit)) return 0;
    this->buffer[len++] = digit;
    length += (digit & 0x7f) * multiplier;
    multiplier <<= 7;
  } while ((digit & 0x80) != 0);

  *lengthLength = len - 1;

  if (isPublish) {
    // Read in topic length to calculate bytes to skip over for Stream writing
    if (!this->readByte(this->buffer, &len)) return 0;
    if (!this->readByte(this->buffer, &len)) return 0;

    skip = (this->buffer[*lengthLength + 1] << 8) + this->buffer[*lengthLength + 2];
    start = 2;
    if (this->buffer[0] & MQTTQOS1) skip += 2; // skip message id
  }

  uint32_t idx = len;
  for (uint16_t i = start; i < length; i++) {
    if (!this->readByte(&digit)) return 0;
    if (this->stream) {
        if (isPublish && idx - *lengthLength - 2 > skip) this->stream->write(digit);
    }
    if (len < MQTT_MAX_PACKET_SIZE) {
      this->buffer[len] = digit;
      len++;
    }
    idx++;
  }

  if (!this->stream && (idx > MQTT_MAX_PACKET_SIZE)) len = 0; // This will cause the packet to be ignored.

  return len;
}

boolean PubSubClient::loop() {
  if (!this->connected()) {
    ::strcpy(this->_statusMessage, "Not connected to a server");
    return false;
  }

  unsigned long t = ::millis();
  if ((t - this->lastInActivity > MQTT_KEEPALIVE * 1000UL) || (t - this->lastOutActivity > MQTT_KEEPALIVE * 1000UL)) {
    if (this->pingOutstanding) {
      this->_state = MQTT_CONNECTION_TIMEOUT;
      ::strcpy(this->_statusMessage, "Connection timeout.  Disconnecting.");
      this->_client->stop();
      return false;
    } else {
      this->buffer[0] = MQTTPINGREQ;
      this->buffer[1] = 0;
      this->_client->write(this->buffer, 2);
      this->lastOutActivity = t;
      this->lastInActivity = t;
      this->pingOutstanding = true;
      ::strcpy(this->_statusMessage, "Ping sent.");
    }
  }

  if (this->_client->available()) {
    uint8_t llen;
    uint16_t len = this->readPacket(&llen);
    uint16_t msgId = 0;
    uint8_t* payload;
    if (len > 0) {
      this->lastInActivity = t;
      uint8_t type = this->buffer[0] & 0xF0;

      if (type == MQTTPUBLISH) {
        uint16_t tl = (this->buffer[llen + 1] << 8) + this->buffer[llen + 2]; /* topic length in bytes */
        ::memmove(this->buffer + llen + 2, this->buffer + llen + 3, tl); /* move topic inside buffer 1 byte to front */
        this->buffer[llen + 2 + tl] = 0; /* end the topic as a 'C' string with \x00 */
        char *topic = (char*) this->buffer + llen + 2;
        ::sprintf(this->_statusMessage, "Received publish for topic %s", topic);

        int payloadOffset = llen + 3 + tl;

        // msgId only present for QOS > 0
        if ((this->buffer[0] & 0x06) == MQTTQOS1) {
          msgId = (this->buffer[payloadOffset] << 8) + this->buffer[payloadOffset + 1];
          payloadOffset += 2;

          this->buffer[0] = MQTTPUBACK;
          this->buffer[1] = 2;
          this->buffer[2] = (msgId >> 8);
          this->buffer[3] = (msgId & 0xFF);
          boolean writeResult = this->_client->write(this->buffer, 4);
          if (writeResult) ::sprintf(this->_statusMessage, "PubSubClient::loop: QoS acknkowledgement of message: %d", msgId);
          else ::sprintf(this->_statusMessage, "PubSubClient::loop: Failed to acknowledge QoS request for message: %d", msgId);
          this->lastOutActivity = t;
        }
        
        if (this->callback) {
          //  Everything but a QoS acknowledgement gets forwarded to the callback routing.
          payload = this->buffer + payloadOffset;
          this->callback(topic, payload, len - payloadOffset);
        }
      } else if (type == MQTTPINGREQ) {
        ::strcpy(this->_statusMessage, "PubSubClient::loop: Ping request received");
        this->buffer[0] = MQTTPINGRESP;
        this->buffer[1] = 0;
        boolean writeResult = this->_client->write(this->buffer, 2);
        if (writeResult) ::strcpy(this->_statusMessage, "PubSubClient::loop: Ping reponse sent");
        else ::strcpy(this->_statusMessage, "PubSubClient::loop: Failed to respond to ping request");
      } else if (type == MQTTPINGRESP) {
        ::strcpy(this->_statusMessage, "PubSubClient::loop: Ping response received");
        this->pingOutstanding = false;
      } else if (type == MQTTSUBACK) ::sprintf(this->_statusMessage, "PubSubClient::loop: Subscription acknowledged.");
      else ::sprintf(this->_statusMessage, "PubSubClient::loop: Unknown message type received: %d", type);
    } else if (!this->connected()) {
      ::strcpy(this->_statusMessage, "PubSubClient::loop: readPacket has closed the connection");
      return false;
    }
  }

  return true;
}

boolean PubSubClient::publish(const char* topic, const char* payload) {
    return this->publish(topic, (const uint8_t*)payload, payload ? ::strlen(payload) : 0, false);
}

boolean PubSubClient::publish(const char* topic, const char* payload, boolean retained) {
    return this->publish(topic, (const uint8_t*)payload, payload ? ::strlen(payload) : 0, retained);
}

boolean PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength) {
    return this->publish(topic, payload, plength, false);
}

boolean PubSubClient::publish(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained) {
  if (!this->connected()) {
    ::strcpy(this->_statusMessage, "Not connected to a server");
    return false;
  }
    
  size_t topicLength = ::strlen(topic);
  if (MQTT_MAX_PACKET_SIZE < MQTT_MAX_HEADER_SIZE + 2 + topicLength + plength) {
    ::sprintf(this->_statusMessage, "PubSubClient::publish: Payload was too long to send.  Max length %d, required length %d", MQTT_MAX_PACKET_SIZE, MQTT_MAX_HEADER_SIZE + 2 + topicLength + plength);
    return false; 
  }

  //  First MQTT_MAX_HEADER_SIZE bytes are the fixed PUBLISH header
  //  Then variable length topic as UTF characters.
  uint16_t length = this->writeString(topic, this->buffer, MQTT_MAX_HEADER_SIZE); // Leave room in the buffer for header and variable length field
  //  Then the packet identifier if QoS > 0
  //  Then the payload
  ::memcpy(this->buffer + length, payload, plength);

  uint8_t header = MQTTPUBLISH;
  if (retained) header |= 1;

//  boolean writeResult = this->write(header, this->buffer, length - MQTT_MAX_HEADER_SIZE);
  boolean writeResult = this->write(header, this->buffer, length + plength);
  if (writeResult) ::sprintf(this->_statusMessage, "M%d: Published to %s", this->nextMsgId, topic);
  else ::sprintf(this->_statusMessage, "M%d: Publish to %s failed", this->nextMsgId, topic);

  return writeResult;
}

boolean PubSubClient::publish_P(const char* topic, const char* payload, boolean retained) {
    return this->publish_P(topic, (const uint8_t*)payload, ::strlen(payload), retained);
}

boolean PubSubClient::publish_P(const char* topic, const uint8_t* payload, unsigned int plength, boolean retained) {
    if (!this->connected()) return false;

    uint8_t llen = 0;
    uint8_t digit;
    unsigned int rc = 0;
    uint16_t tlen = ::strlen(topic);
    unsigned int pos = 0;
    unsigned int i;
    uint8_t header = MQTTPUBLISH;
    unsigned int len;

    if (retained) header |= 1;
    this->buffer[pos++] = header;
    len = plength + 2 + tlen;
    do {
        digit = len & 0x7f;
        len >>= 7;
        if (len > 0) digit |= 0x80;
        this->buffer[pos++] = digit;
        llen++;
    } while (len > 0);

    pos = this->writeString(topic, this->buffer, pos);
    rc += this->_client->write(this->buffer, pos);
    for (i=0; i < plength; i++) rc += this->_client->write((char)::pgm_read_byte_near(payload + i));

    this->lastOutActivity = ::millis();
    int expectedLength = 1 + llen + 2 + tlen + plength;

    return (rc == expectedLength);
}

boolean PubSubClient::beginPublish(const char* topic, unsigned int plength, boolean retained) {
    if (connected()) {
        // Send the header and variable length field
        uint16_t length = MQTT_MAX_HEADER_SIZE;
        length = this->writeString(topic, this->buffer, length);
        uint8_t header = MQTTPUBLISH;
        if (retained) header |= 1;
        size_t hlen = this->buildHeader(header, this->buffer, plength + length - MQTT_MAX_HEADER_SIZE);
        uint16_t rc = this->_client->write(this->buffer + (MQTT_MAX_HEADER_SIZE - hlen), length - (MQTT_MAX_HEADER_SIZE - hlen));
        this->lastOutActivity = ::millis();
        return (rc == (length - (MQTT_MAX_HEADER_SIZE - hlen)));
    }
    return false;
}

int PubSubClient::endPublish() {
 return 1;
}

size_t PubSubClient::write(uint8_t data) {
    this->lastOutActivity = ::millis();
    return this->_client->write(data);
}

size_t PubSubClient::write(const uint8_t* writeBuffer, size_t size) {
  this->lastOutActivity = ::millis();
  ::sprintf(this->_statusMessage, "Wrote %d, length %d", writeBuffer, size);
  return this->_client->write(writeBuffer, size);
}

size_t PubSubClient::buildHeader(uint8_t header, uint8_t* buf, uint16_t length) {
  uint8_t lenBuf[4];
  uint8_t llen = 0;
  uint8_t digit;
  uint8_t pos = 0;
  uint16_t len = length;
  do {
    digit = len & 0x7f;
    len >>= 7;
    if (len > 0) digit |= 0x80;
    lenBuf[pos++] = digit;
    llen++;
  } while (len > 0);

  buf[4 - llen] = header;
  ::memcpy(&buf[MQTT_MAX_HEADER_SIZE - llen], lenBuf, llen);

  return llen + 1; // Full header size is variable length bit plus the 1-byte fixed header
}

boolean PubSubClient::write(uint8_t header, uint8_t* buf, uint16_t length) {
  uint16_t rc;
  uint8_t hlen = this->buildHeader(header, buf, length);

#ifdef MQTT_MAX_TRANSFER_SIZE
  uint8_t* writeBuf = buf + (MQTT_MAX_HEADER_SIZE - hlen);
  uint16_t bytesRemaining = length + hlen;  //  Match the length type
  uint8_t bytesToWrite;
  boolean result = true;
  while((bytesRemaining > 0) && result) {
    bytesToWrite = (bytesRemaining > MQTT_MAX_TRANSFER_SIZE) ? MQTT_MAX_TRANSFER_SIZE : bytesRemaining;
    rc = this->_client->write(writeBuf, bytesToWrite);
    result = (rc == bytesToWrite);
    bytesRemaining -= rc;
    writeBuf += rc;
  }

  return result;
#else
  rc = this->_client->write(buf + (MQTT_MAX_HEADER_SIZE - hlen), length + hlen);
  this->lastOutActivity = ::millis();
  return (rc == hlen + length);
#endif
}

boolean PubSubClient::subscribe(const char* topic) {
    return subscribe(topic, 0);
}

boolean PubSubClient::subscribe(const char* topic, uint8_t qos) {
  if (qos > 1) {
    ::sprintf(this->_statusMessage, "PubSubClient::subscribe: qos %d is greater than 1 and not supported", qos);
    return false;
  }

  if (MQTT_MAX_PACKET_SIZE < 9 + ::strlen(topic)) {
    ::sprintf(this->_statusMessage, "PubSubClient::subscribe: topic length %d is greater than %d and not supported", ::strlen(topic), MQTT_MAX_PACKET_SIZE - 9);
    return false; // Too long
  }

  if (!this->connected()) return false;

  // Leave room in the buffer for header and variable length field
  uint16_t length = MQTT_MAX_HEADER_SIZE;
  this->nextMsgId++;
  if (this->nextMsgId == 0) this->nextMsgId = 1;
  this->buffer[length++] = (this->nextMsgId >> 8);
  this->buffer[length++] = (this->nextMsgId & 0xFF);
  length = this->writeString((char*)topic, this->buffer, length);
  this->buffer[length++] = qos;

  bool writeResult = this->write(MQTTSUBSCRIBE | MQTTQOS1, this->buffer, length - MQTT_MAX_HEADER_SIZE);
  if (writeResult) ::sprintf(this->_statusMessage, "M%d: Subscription sent", this->nextMsgId);
  else ::sprintf(this->_statusMessage, "M%d: Subscription to %s not sent", this->nextMsgId, topic);
  return writeResult;
}

boolean PubSubClient::unsubscribe(const char* topic) {
  if (MQTT_MAX_PACKET_SIZE < 9 + ::strlen(topic)) {
    ::sprintf(this->_statusMessage, "PubSubClient::unsubscribe: topic length %d is greater than %d and not supported", ::strlen(topic), MQTT_MAX_PACKET_SIZE - 9);
    return false;
  }

  if (!this->connected())  return false;

  uint16_t length = MQTT_MAX_HEADER_SIZE;
  this->nextMsgId++;
  if (this->nextMsgId == 0) this->nextMsgId = 1;
  this->buffer[length++] = (this->nextMsgId >> 8);
  this->buffer[length++] = (this->nextMsgId & 0xFF);
  length = this->writeString(topic, this->buffer, length);

  bool writeResult = this->write(MQTTUNSUBSCRIBE | MQTTQOS1, this->buffer, length - MQTT_MAX_HEADER_SIZE);
  if (writeResult) ::sprintf(this->_statusMessage, "M%d: Unsubscribe request sent", this->nextMsgId);
  else ::sprintf(this->_statusMessage, "M%d: Unsubscribe from %s not sent", this->nextMsgId, topic);
  return writeResult;
}

void PubSubClient::disconnect() {
  this->buffer[0] = MQTTDISCONNECT;
  this->buffer[1] = 0;
  this->_client->write(buffer, 2);
  this->_state = MQTT_DISCONNECTED;
  this->_client->flush();
  this->_client->stop();
  this->lastInActivity = this->lastOutActivity = ::millis();

  ::strcpy(this->_statusMessage, "PubSubClient disconnected");
}

uint16_t PubSubClient::writeString(const char* string, uint8_t* buf, uint16_t pos) {
  uint16_t stringLength = ::strlen(string);
  buf[pos] = (stringLength >> 8);
  buf[pos + 1] = (stringLength & 0xFF);
  ::memcpy(buf + pos + 2, string, stringLength);

  return pos + 2 + stringLength;
}


boolean PubSubClient::connected() {
  if (NULL == this->_client) return false;

  if (this->_client->connected()) {
    this->_statusMessage[0] = 0;
    return true;
  }

  //  Not connected.
    if (this->_state == MQTT_CONNECTED) {
      this->_state = MQTT_CONNECTION_LOST;
      this->_client->flush();
      this->_client->stop();
    }
    return false;
}

const char* PubSubClient::getLastStatus() { return this->_statusMessage; }

PubSubClient& PubSubClient::setServer(uint8_t * ip, uint16_t port) {
  IPAddress addr(ip[0],ip[1],ip[2],ip[3]);
  return this->setServer(addr, port);
}

PubSubClient& PubSubClient::setServer(IPAddress ip, uint16_t port) {
  this->ip = ip;
  this->port = port;
  this->domain = NULL;
  return *this;
}

PubSubClient& PubSubClient::setServer(const char * domain, uint16_t port) {
  this->domain = domain;
  this->port = port;
  return *this;
}

PubSubClient& PubSubClient::setCallback(MQTT_CALLBACK_SIGNATURE) {
  this->callback = callback;
  return *this;
}

PubSubClient& PubSubClient::setClient(Client& client) {
  this->_client = &client;
  return *this;
}

PubSubClient& PubSubClient::setStream(Stream& stream) {
  this->stream = &stream;
  return *this;
}

int PubSubClient::state() {
  return this->_state;
}