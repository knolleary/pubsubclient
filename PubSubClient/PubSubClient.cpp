/*
 PubSubClient.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include <EthernetClient.h>
#include <string.h>

PubSubClient::PubSubClient() : _client() {
}

PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int)) : _client() {
   this->callback = callback;
   this->ip = ip;
   this->port = port;
}

PubSubClient::PubSubClient(char* domain, uint16_t port, void (*callback)(char*,uint8_t*,unsigned int)) : _client() {
   this->callback = callback;
   this->domain = domain;
   this->port = port;
}

boolean PubSubClient::connect(char *id) {
   return connect(id,0,0,0,0);
}

boolean PubSubClient::connect(char *id, char* willTopic, uint8_t willQos, uint8_t willRetain, char* willMessage) {
   if (!connected()) {
      int result = 0;
      
      if (domain != NULL) {
        result = _client.connect(this->domain, this->port);
      } else {
        result = _client.connect(this->ip, this->port);
      }
      
      if (result) {
         nextMsgId = 1;
         uint8_t d[9] = {0x00,0x06,'M','Q','I','s','d','p',MQTTPROTOCOLVERSION};
         uint8_t length = 0;
         unsigned int j;
         for (j = 0;j<9;j++) {
            buffer[length++] = d[j];
         }
         if (willTopic) {
            buffer[length++] = 0x06|(willQos<<3)|(willRetain<<5);
         } else {
            buffer[length++] = 0x02;
         }
         buffer[length++] = ((MQTT_KEEPALIVE) >> 8);
         buffer[length++] = ((MQTT_KEEPALIVE) & 0xff);
         length = writeString(id,buffer,length);
         if (willTopic) {
            length = writeString(willTopic,buffer,length);
            length = writeString(willMessage,buffer,length);
         }
         write(MQTTCONNECT,buffer,length);
         lastOutActivity = millis();
         lastInActivity = millis();
         while (!_client.available()) {
            unsigned long t= millis();
            if (t-lastInActivity > MQTT_KEEPALIVE*1000) {
               _client.stop();
               return false;
            }
         }
         uint16_t len = readPacket();
         
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

uint16_t PubSubClient::readPacket() {
   uint16_t len = 0;
   buffer[len++] = readByte();
   uint8_t multiplier = 1;
   uint16_t length = 0;
   uint8_t digit = 0;
   do {
      digit = readByte();
      buffer[len++] = digit;
      length += (digit & 127) * multiplier;
      multiplier *= 128;
   } while ((digit & 128) != 0);
   
   for (uint16_t i = 0;i<length;i++)
   {
      if (len < MQTT_MAX_PACKET_SIZE) {
         buffer[len++] = readByte();
      } else {
         readByte();
         len = 0; // This will cause the packet to be ignored.
      }
   }

   return len;
}

boolean PubSubClient::loop() {
   if (connected()) {
      unsigned long t = millis();
      if ((t - lastInActivity > MQTT_KEEPALIVE*1000) || (t - lastOutActivity > MQTT_KEEPALIVE*1000)) {
         if (pingOutstanding) {
            _client.stop();
            return false;
         } else {
            _client.write(MQTTPINGREQ);
            _client.write((uint8_t)0);
            lastOutActivity = t;
            lastInActivity = t;
            pingOutstanding = true;
         }
      }
      if (_client.available()) {
         uint16_t len = readPacket();
         if (len > 0) {
            lastInActivity = t;
            uint8_t type = buffer[0]&0xF0;
            if (type == MQTTPUBLISH) {
               if (callback) {
                  uint16_t tl = (buffer[2]<<8)+buffer[3];
                  char topic[tl+1];
                  for (uint16_t i=0;i<tl;i++) {
                     topic[i] = buffer[4+i];
                  }
                  topic[tl] = 0;
                  // ignore msgID - only support QoS 0 subs
                  uint8_t *payload = buffer+4+tl;
                  callback(topic,payload,len-4-tl);
               }
            } else if (type == MQTTPINGREQ) {
               _client.write(MQTTPINGRESP);
               _client.write((uint8_t)0);
            } else if (type == MQTTPINGRESP) {
               pingOutstanding = false;
            }
         }
      }
      return true;
   }
   return false;
}

boolean PubSubClient::publish(char* topic, char* payload) {
   return publish(topic,(uint8_t*)payload,strlen(payload),false);
}

boolean PubSubClient::publish(char* topic, uint8_t* payload, unsigned int plength) {
   return publish(topic, payload, plength, false);
}

boolean PubSubClient::publish(char* topic, uint8_t* payload, unsigned int plength, boolean retained) {
   if (connected()) {
      uint16_t length = writeString(topic,buffer,false);
      uint16_t i;
      for (i=0;i<plength;i++) {
         buffer[length++] = payload[i];
      }
      uint8_t header = MQTTPUBLISH;
      if (retained) {
         header |= 1;
      }
      return write(header,buffer,length);
   }
   return false;
}


boolean PubSubClient::write(uint8_t header, uint8_t* buf, uint16_t length) {
   uint8_t lenBuf[4];
   uint8_t llen = 0;
   uint8_t digit;
   uint8_t pos = 0;
   uint8_t rc;
   uint8_t len = length;
   do {
      digit = len % 128;
      len = len / 128;
      if (len > 0) {
         digit |= 0x80;
      }
      lenBuf[pos++] = digit;
      llen++;
   } while(len>0);

   rc = _client.write(header);
   rc += _client.write(lenBuf,llen);
   rc += _client.write(buf,length);
   lastOutActivity = millis();
   return (rc == 1+llen+length);
}


boolean PubSubClient::subscribe(char* topic) {
   if (connected()) {
      uint16_t length = 2;
      nextMsgId++;
      if (nextMsgId == 0) {
         nextMsgId = 1;
      }
      buffer[0] = nextMsgId >> 8;
      buffer[1] = nextMsgId - (buffer[0]<<8);
      length = writeString(topic, buffer,length);
      buffer[length++] = 0; // Only do QoS 0 subs
      return write(MQTTSUBSCRIBE|MQTTQOS1,buffer,length);
   }
   return false;
}

void PubSubClient::disconnect() {
   _client.write(MQTTDISCONNECT);
   _client.write((uint8_t)0);
   _client.stop();
   lastInActivity = millis();
   lastOutActivity = millis();
}

uint16_t PubSubClient::writeString(char* string, uint8_t* buf, uint16_t pos) {
   char* idp = string;
   uint16_t i = 0;
   pos += 2;
   while (*idp) {
      buf[pos++] = *idp++;
      i++;
   }
   buf[pos-i-2] = 0;
   buf[pos-i-1] = i;
   return pos;
}


boolean PubSubClient::connected() {
   int rc = (int)_client.connected();
   if (!rc) _client.stop();
   return rc;
}



