/*
 PubSubClient.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "WConstants.h"
#include "PubSubClient.h"
#include "Client.h"
#include "string.h"

#define MQTTCONNECT 1<<4
#define MQTTPUBLISH 3<<4
#define MQTTSUBSCRIBE 8<<4

PubSubClient::PubSubClient(uint8_t *ip, uint16_t port, void (*callback)(char*,uint8_t*,int)) : _client(ip,port) {
   this->callback = callback;
}
int PubSubClient::connect(char *id) {
   return connect(id,0,0,0,0);
}

int PubSubClient::connect(char *id, char* willTopic, uint8_t willQos, uint8_t willRetain, char* willMessage) {
   if (!connected()) {
      if (_client.connect()) {
         nextMsgId = 1;
         uint8_t d[9] = {0x00,0x06,0x4d,0x51,0x49,0x73,0x64,0x70,0x03};
         uint8_t length = 0;
         int j;
         for (j = 0;j<9;j++) {
            buffer[length++] = d[j];
         }
         if (willTopic) {
            buffer[length++] = 0x06|(willQos<<3)|(willRetain<<5);
         } else {
            buffer[length++] = 0x02;
         }
         buffer[length++] = 0;
         buffer[length++] = (KEEPALIVE/1000);
         length = writeString(id,buffer,length);
         if (willTopic) {
            length = writeString(willTopic,buffer,length);
            length = writeString(willMessage,buffer,length);
         }
         write(MQTTCONNECT,buffer,length);
         while (!_client.available()) {}
         uint8_t len = readPacket();
         
         if (len == 4 && buffer[3] == 0) {
            lastActivity = millis();
            return 1;
         }
      }
      _client.stop();
   }
   return 0;
}

uint8_t PubSubClient::readByte() {
   while(!_client.available()) {}
   return _client.read();
}

uint8_t PubSubClient::readPacket() {
   uint8_t len = 0;
   buffer[len++] = readByte();
   uint8_t multiplier = 1;
   uint8_t length = 0;
   uint8_t digit = 0;
   do {
      digit = readByte();
      buffer[len++] = digit;
      length += (digit & 127) * multiplier;
      multiplier *= 128;
   } while ((digit & 128) != 0);
   
   for (int i = 0;i<length;i++)
   {
      if (len < MAX_PACKET_SIZE) {
         buffer[len++] = readByte();
      } else {
         readByte();
         len = 0; // This will cause the packet to be ignored.
      }
   }

   return len;
}

int PubSubClient::loop() {
   if (connected()) {
      long t = millis();
      if (t - lastActivity > KEEPALIVE) {
         _client.write(192);
         _client.write((uint8_t)0);
         lastActivity = t;
      }
      if (_client.available()) {
         uint8_t len = readPacket();
         if (len > 0) {
            uint8_t type = buffer[0]>>4;
            if (type == 3) { // PUBLISH
               if (callback) {
                  uint8_t tl = (buffer[2]<<3)+buffer[3];
                  char topic[tl+1];
                  for (int i=0;i<tl;i++) {
                     topic[i] = buffer[4+i];
                  }
                  topic[tl] = 0;
                  // ignore msgID - only support QoS 0 subs
                  uint8_t *payload = buffer+4+tl;
                  callback(topic,payload,len-4-tl);
               }
            } else if (type == 12) { // PINGREG
               _client.write(208);
               _client.write((uint8_t)0);
               lastActivity = t;
            }
         }
      }
      return 1;
   }
   return 0;
}

int PubSubClient::publish(char* topic, char* payload) {
   return publish(topic,(uint8_t*)payload,strlen(payload));
}


int PubSubClient::publish(char* topic, uint8_t* payload, uint8_t plength) {
   if (connected()) {
      uint8_t length = writeString(topic,buffer,0);
      int i;
      for (i=0;i<plength;i++) {
         buffer[length++] = payload[i];
      }
      //header |= 1; retain
      write(MQTTPUBLISH,buffer,length);
      return 1;
   }
   return 0;
}


int PubSubClient::write(uint8_t header, uint8_t* buf, uint8_t length) {
   _client.write(header);
   _client.write(length);
   for (int i=0;i<length;i++) {
      _client.write(buf[i]);
   }
   return 0;
}


void PubSubClient::subscribe(char* topic) {
   if (connected()) {
      uint8_t length = 2;
      nextMsgId++;
      buffer[0] = nextMsgId >> 8;
      buffer[1] = nextMsgId - (buffer[0]<<8);
      length = writeString(topic, buffer,length);
      buffer[length++] = 0; // Only do QoS 0 subs
      write(MQTTSUBSCRIBE,buffer,length);
   }
}

void PubSubClient::disconnect() {
   _client.write(224);
   _client.write((uint8_t)0);
   _client.stop();
   lastActivity = millis();
}

uint8_t PubSubClient::writeString(char* string, uint8_t* buf, uint8_t pos) {
   char* idp = string;
   uint8_t i = 0;
   pos += 2;
   while (*idp) {
      buf[pos++] = *idp++;
      i++;
   }
   buf[pos-i-2] = 0;
   buf[pos-i-1] = i;
   return pos;
}


int PubSubClient::connected() {
   int rc = (int)_client.connected();
   if (!rc) _client.stop();
   return rc;
}



