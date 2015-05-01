/*
 PubSubClient.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include <string.h>

PubSubClient::PubSubClient(IPAddress &ip, uint16_t port) :
  _callback(NULL),
  _stream(NULL),
  server_ip(ip),
  server_port(port)
{}

PubSubClient::PubSubClient(String hostname, uint16_t port) :
  _callback(NULL),
  _stream(NULL),
  server_port(port),
  server_hostname(hostname)
{}

PubSubClient& PubSubClient::set_auth(String u, String p) {
  username = u;
  password = p;
  return *this;
}

PubSubClient& PubSubClient::unset_auth(void) {
  username = "";
  password = "";
  return *this;
}

PubSubClient& PubSubClient::set_callback(callback_t cb) {
  _callback = cb;
  return *this;
}

PubSubClient& PubSubClient::unset_callback(void) {
  _callback = NULL;
  return *this;
}

PubSubClient& PubSubClient::set_stream(Stream &s) {
  _stream = &s;
  return *this;
}

PubSubClient& PubSubClient::unset_stream(void) {
  _stream = NULL;
  return *this;
}

bool PubSubClient::connect(String id) {
  return connect(id, "", 0, false, "");
}

bool PubSubClient::connect(String id, String willTopic, uint8_t willQos, bool willRetain, String willMessage) {
   if (!connected()) {
      int result = 0;
      
      if (server_hostname.length() > 0)
        result = _client.connect(server_hostname.c_str(), server_port);
      else
        result = _client.connect(server_ip, server_port);

      if (result) {
         nextMsgId = 1;
         uint8_t d[9] = {0x00,0x06,'M','Q','I','s','d','p',MQTTPROTOCOLVERSION};
         // Leave room in the buffer for header and variable length field
         uint16_t length = 5;
	 memcpy(buffer + length, d, 9);
	 length += 9;

         uint8_t v;
         if (willTopic.length()) {
	   if (willQos > 2)
	     willQos = 2;
	   v = 0x06 | (willQos << 3) | (willRetain << 5);
	 } else
            v = 0x02;

         if (username.length()) {
            v = v | 0x80;
            if (password.length())
               v = v | 0x40;
         }

         buffer[length++] = v;

         buffer[length++] = ((MQTT_KEEPALIVE) >> 8);
         buffer[length++] = ((MQTT_KEEPALIVE) & 0xFF);
         length = writeString(id, buffer, length);
         if (willTopic.length()) {
            length = writeString(willTopic, buffer, length);
            length = writeString(willMessage, buffer, length);
         }

         if (username.length()) {
            length = writeString(username, buffer, length);
            if (password.length())
	      length = writeString(password, buffer, length);
         }
         
         write(MQTTCONNECT,buffer,length-5);
         
         lastInActivity = lastOutActivity = millis();
         
         while (!_client.available()) {
            unsigned long t = millis();
            if (t-lastInActivity > MQTT_KEEPALIVE*1000UL) {
               _client.stop();
               return false;
            }
         }
         uint8_t llen;
         uint16_t len = readPacket(&llen);
         
         if (len == 4 && buffer[3] == 0) {
            lastInActivity = millis();
            pingOutstanding = false;
            return true;
         }
      }
      _client.stop();
   }
   return false;
}

uint8_t PubSubClient::readByte() {
   while(!_client.available()) {}
   return _client.read();
}

uint16_t PubSubClient::readPacket(uint8_t* lengthLength) {
   uint16_t len = 0;
   buffer[len++] = readByte();
   bool isPublish = (buffer[0]&0xF0) == MQTTPUBLISH;
   uint8_t shifter = 0;
   uint16_t length = 0;
   uint8_t digit = 0;
   uint16_t skip = 0;
   uint8_t start = 0;
   
   do {
      digit = readByte();
      buffer[len++] = digit;
      length += (digit & 0x7f) << shifter;
      shifter += 7;
   } while (digit & 0x80);
   *lengthLength = len-1;

   if (isPublish) {
      // Read in topic length to calculate bytes to skip over for Stream writing
      buffer[len++] = readByte();
      buffer[len++] = readByte();
      skip = (buffer[*lengthLength+1] << 8) | buffer[*lengthLength + 2];
      start = 2;
      if (buffer[0]&MQTTQOS1) {
         // skip message id
         skip += 2;
      }
   }

   for (uint16_t i = start;i<length;i++) {
      digit = readByte();
      if (_stream)
	if (isPublish && len-*lengthLength-2>skip)
	_client.write(digit);
      if (len < MQTT_MAX_PACKET_SIZE) {
         buffer[len] = digit;
      }
      len++;
   }
   
   if ((!_stream) && (len > MQTT_MAX_PACKET_SIZE)) {
       len = 0; // This will cause the packet to be ignored.
   }

   return len;
}

bool PubSubClient::loop() {
   if (connected()) {
      unsigned long t = millis();
      if ((t - lastInActivity > MQTT_KEEPALIVE*1000UL) || (t - lastOutActivity > MQTT_KEEPALIVE*1000UL)) {
         if (pingOutstanding) {
            _client.stop();
            return false;
         } else {
            buffer[0] = MQTTPINGREQ;
            buffer[1] = 0;
            _client.write(buffer,2);
            lastOutActivity = t;
            lastInActivity = t;
            pingOutstanding = true;
         }
      }
      if (_client.available()) {
         uint8_t llen;
         uint16_t len = readPacket(&llen);
         uint16_t msgId = 0;
         uint8_t *payload;
         if (len > 0) {
            lastInActivity = t;
            uint8_t type = buffer[0]&0xF0;
            if (type == MQTTPUBLISH) {
               if (_callback) {
                  uint16_t tl = (buffer[llen + 1] << 8) | buffer[llen + 2];
                  char topic[tl + 1];
		  memcpy(topic, buffer + llen + 3, tl);
                  topic[tl] = 0;
                  // msgId only present for QOS>0
                  if ((buffer[0]&0x06) == MQTTQOS1) {
                    msgId = (buffer[llen + 3 + tl] << 8) | buffer[llen + 3 + tl + 1];
                    payload = buffer+llen+3+tl+2;
                    _callback(topic,payload,len-llen-3-tl-2);
                    
                    buffer[0] = MQTTPUBACK;
                    buffer[1] = 2;
                    buffer[2] = (msgId >> 8);
                    buffer[3] = (msgId & 0xFF);
                    _client.write(buffer, 4);
                    lastOutActivity = t;

                  } else {
                    payload = buffer+llen+3+tl;
                    _callback(topic,payload,len-llen-3-tl);
                  }
               }
            } else if (type == MQTTPINGREQ) {
               buffer[0] = MQTTPINGRESP;
               buffer[1] = 0;
               _client.write(buffer, 2);
            } else if (type == MQTTPINGRESP) {
               pingOutstanding = false;
            }
         }
      }
      return true;
   }
   return false;
}

bool PubSubClient::publish(String topic, String payload) {
  return publish(topic, (const uint8_t*)payload.c_str(), payload.length(), false);
}

bool PubSubClient::publish(String topic, const uint8_t* payload, unsigned int plength, bool retained) {
   if (connected()) {
      // Leave room in the buffer for header and variable length field
      uint16_t length = 5;
      length = writeString(topic, buffer, length);

      memcpy(buffer + length, payload, plength);
      length += plength;

      uint8_t header = MQTTPUBLISH;
      if (retained) {
         header |= 1;
      }
      return write(header,buffer,length-5);
   }
   return false;
}

bool PubSubClient::publish_P(String topic, const uint8_t* PROGMEM payload, unsigned int plength, bool retained) {
   uint8_t llen = 0;
   uint8_t digit;
   unsigned int rc = 0;
   uint16_t tlen;
   unsigned int pos = 0;
   unsigned int i;
   uint8_t header;
   unsigned int len;
   
   if (!connected()) {
      return false;
   }
   
   tlen = topic.length();
   
   header = MQTTPUBLISH;
   if (retained) {
      header |= 1;
   }
   buffer[pos++] = header;
   len = plength + 2 + tlen;
   do {
      digit = len & 0x7f;
      len >>= 7;
      if (len)
         digit |= 0x80;
      buffer[pos++] = digit;
      llen++;
   } while (len);
   
   pos = writeString(topic,buffer,pos);
   
   rc += _client.write(buffer, pos);
   
   for (i=0;i<plength;i++) {
      rc += _client.write((char)pgm_read_byte_near(payload + i));
   }
   
   lastOutActivity = millis();
   
   return rc == tlen + 4 + plength;
}

bool PubSubClient::write(uint8_t header, uint8_t* buf, uint16_t length) {
   uint8_t lenBuf[4];
   uint8_t llen = 0;
   uint8_t digit;
   uint8_t pos = 0;
   uint8_t rc;
   uint8_t len = length;
   do {
      digit = len & 0x7f;
      len >>= 7;
      if (len)
         digit |= 0x80;
      lenBuf[pos++] = digit;
      llen++;
   } while (len);

   buf[4-llen] = header;
   memcpy(buf + 5 - llen, lenBuf, llen);
   rc = _client.write(buf + 4 - llen, length + 1 + llen);
   
   lastOutActivity = millis();
   return (rc == 1+llen+length);
}

bool PubSubClient::subscribe(String topic, uint8_t qos) {
   if (qos < 0 || qos > 1)
     return false;

   if (connected()) {
      // Leave room in the buffer for header and variable length field
      uint16_t length = 5;
      nextMsgId++;
      if (nextMsgId == 0) {
         nextMsgId = 1;
      }
      buffer[length++] = (nextMsgId >> 8);
      buffer[length++] = (nextMsgId & 0xFF);
      length = writeString(topic, buffer,length);
      buffer[length++] = qos;
      return write(MQTTSUBSCRIBE|MQTTQOS1,buffer,length-5);
   }
   return false;
}

bool PubSubClient::unsubscribe(String topic) {
   if (connected()) {
      uint16_t length = 5;
      nextMsgId++;
      if (nextMsgId == 0) {
         nextMsgId = 1;
      }
      buffer[length++] = (nextMsgId >> 8);
      buffer[length++] = (nextMsgId & 0xFF);
      length = writeString(topic, buffer,length);
      return write(MQTTUNSUBSCRIBE|MQTTQOS1,buffer,length-5);
   }
   return false;
}

void PubSubClient::disconnect() {
   buffer[0] = MQTTDISCONNECT;
   buffer[1] = 0;
   _client.write(buffer, 2);
   _client.stop();
   lastInActivity = lastOutActivity = millis();
}

uint16_t PubSubClient::writeString(String string, uint8_t* buf, uint16_t pos) {
   const char* idp = string.c_str();
   uint16_t i = 0;
   pos += 2;
   while (*idp) {
      buf[pos++] = *idp++;
      i++;
   }
   buf[pos-i-2] = (i >> 8);
   buf[pos-i-1] = (i & 0xFF);
   return pos;
}


bool PubSubClient::connected() {
   bool rc = _client.connected();
   if (!rc)
     _client.stop();

   return rc;
}

