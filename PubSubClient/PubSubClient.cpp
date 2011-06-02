/*
 PubSubClient.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include "Client.h"
#include "string.h"

PubSubClient::PubSubClient() : _client(0) {
}

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
         uint8_t d[9] = {0x00,0x06,'M','Q','I','s','d','p',MQTTPROTOCOLVERSION};
         uint16_t packet_len = 12 + strlen(id) + 2;
         if (willTopic) {
             packet_len += strlen(willTopic) + 2 + strlen(willMessage) + 2;
         }
         
         _client.write(MQTTCONNECT);
         writeRemainingLength(packet_len);
         _client.write(d, sizeof(d));
         if (willTopic) {
             _client.write(0x06|(willQos<<3)|(willRetain<<5));
         } else {
             _client.write(0x02);
         }

         // FIXME - should do proper 16bit write
         _client.write((uint8_t)0);
         _client.write(KEEPALIVE/1000);
         writeString(id, strlen(id));
         if (willTopic) {
             writeString(willTopic, strlen(willTopic));
             writeString(willMessage, strlen(willMessage));
         }
         lastOutActivity = millis();
         lastInActivity = millis();
         while (!_client.available()) {
            long t= millis();
            if (t-lastInActivity > KEEPALIVE) {
               _client.stop();
               return ERR_TIMEOUT_EXCEEDED;
            }
         }
         uint16_t len = readPacket();
         
         if (len == 4 && buffer[3] == 0) {
            lastInActivity = millis();
            pingOutstanding = false;
            return ERR_OK;
         }
      }
      _client.stop();
   }
   return ERR_NOT_CONNECTED;
}

uint8_t PubSubClient::readByte() {
   while(!_client.available()) {}
   return _client.read();
}

uint16_t PubSubClient::readPacket() {
   uint16_t len = 0;
   buffer[len++] = readByte();
   uint16_t multiplier = 1;
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
      if ((t - lastInActivity > KEEPALIVE) || (t - lastOutActivity > KEEPALIVE)) {
         if (pingOutstanding) {
            _client.stop();
            return ERR_TIMEOUT_EXCEEDED;
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
            } else if (type == MQTTPINGREQ) {
               _client.write(MQTTPINGRESP);
               _client.write((uint8_t)0);
            } else if (type == MQTTPINGRESP) {
               pingOutstanding = false;
            }
         }
      }
      return ERR_OK;
   }
   return ERR_NOT_CONNECTED;
}

int PubSubClient::publish(char* topic, char* payload) {
   return publish(topic,(uint8_t*)payload,strlen(payload));
}

int PubSubClient::publish(char* topic, uint8_t* payload, uint16_t plength) {
   return publish(topic, payload, plength, 0);
}

int PubSubClient::publish(char* topic, uint8_t* payload, uint16_t plength, uint8_t retained) {
   if (connected()) {
      uint8_t header = MQTTPUBLISH;
      if (retained != 0) {
         header |= 1;
      }
      _client.write(header);
      // topic needs to be preceeded by two bytes indicating the length of that string
      uint16_t topic_len = strlen(topic);
      writeRemainingLength(2 + topic_len + plength);      
      writeString(topic, topic_len);
      // now finally, payload, using original payload pointer!
      _client.write(payload, plength);
      return ERR_OK;
   }
   return ERR_NOT_CONNECTED;
}

int PubSubClient::subscribe(char* topic) {
   if (connected()) {
      nextMsgId++;
      _client.write(MQTTSUBSCRIBE);
      // 2 for msgid, 2 for utf8 encoding, 1 for qos flags
      writeRemainingLength(2 + strlen(topic) + 2 + 1);
      _client.write(nextMsgId >> 8);
      _client.write(nextMsgId & 0x00ff);
      writeString(topic, strlen(topic));
      _client.write((uint8_t)0);  // only do QoS 0 subs
      return ERR_OK;
   }
   return ERR_NOT_CONNECTED;
}

void PubSubClient::disconnect() {
   _client.write(MQTTDISCONNECT);
   _client.write((uint8_t)0);
   _client.stop();
   lastInActivity = millis();
   lastOutActivity = millis();
}

int PubSubClient::writeString(char *string, uint16_t strlen) {
      // big endian string length first...
      _client.write(strlen >> 8);
      _client.write(strlen & 0x00ff);
      _client.write((uint8_t *)string, strlen);
      return 0;
}

/**
 * For a given "length" write to the client the variable width field "remainging length"
 * in the "fixed" header.  (Isn't that wonderfully clear? :)
 * @param length length of _payload_ plus _variable_ headers
 * @return just 0
 */
int PubSubClient::writeRemainingLength(uint16_t length) {
    uint16_t len_tmp = length;
    uint16_t digit;
    do {
        digit = len_tmp % 128;
        len_tmp = len_tmp / 128;
        // if there are more digits to encode, set the top bit of this digit
        if (len_tmp > 0) {
            digit = digit | 0x80;
        }
        _client.write(digit);
    } while (len_tmp > 0);
    return 0;
}

int PubSubClient::connected() {
   int rc = (int)_client.connected();
   if (!rc) _client.stop();
   return rc;
}



