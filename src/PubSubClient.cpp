/*
 PubSubClient.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include <string.h>

PubSubClient::PubSubClient(IPAddress &ip, uint16_t port) :
  _callback(NULL),
  //  _stream(NULL),
  server_ip(ip),
  server_port(port)
{}

PubSubClient::PubSubClient(String hostname, uint16_t port) :
  _callback(NULL),
  //  _stream(NULL),
  server_port(port),
  server_hostname(hostname)
{}

PubSubClient& PubSubClient::set_callback(callback_t cb) {
  _callback = cb;
  return *this;
}

PubSubClient& PubSubClient::unset_callback(void) {
  _callback = NULL;
  return *this;
}

/*
PubSubClient& PubSubClient::set_stream(Stream &s) {
  _stream = &s;
  return *this;
}

PubSubClient& PubSubClient::unset_stream(void) {
  _stream = NULL;
  return *this;
}
*/

bool PubSubClient::connect(String id) {
  if (connected())
    return false;

  return connect(id, "", 0, false, "");
}

bool PubSubClient::connect(String id, String willTopic, uint8_t willQos, bool willRetain, String willMessage) {
  if (connected())
    return false;

  MQTT::Connect conn(id);
  if (willTopic.length())
    conn.set_will(willTopic, willMessage, willQos, willRetain);
  return connect(conn);
}

bool PubSubClient::connect(MQTT::Connect &conn) {
  if (connected())
    return false;

  int result = 0;

  if (server_hostname.length() > 0)
    result = _client.connect(server_hostname.c_str(), server_port);
  else
    result = _client.connect(server_ip, server_port);

  if (result) {
    nextMsgId = 1;

    conn.send(_client);

    lastInActivity = lastOutActivity = millis();

    keepalive = conn.keepalive();	// Store the keepalive period from this connection
    while (!_client.available()) {
      unsigned long t = millis();
      if (t - lastInActivity > keepalive * 1000UL) {
	_client.stop();
	return false;
      }
    }

    MQTT::Message *msg = MQTT::readPacket(_client);
    bool ret = false;
    if (msg != NULL) {
      ret = msg->type() == MQTTCONNACK;
      free(msg);
    }
    return ret;
  }
  _client.stop();
}

bool PubSubClient::loop() {
  if (!connected())
    return false;

  unsigned long t = millis();
  if ((t - lastInActivity > keepalive * 1000UL) || (t - lastOutActivity > keepalive * 1000UL)) {
    if (pingOutstanding) {
      _client.stop();
      return false;
    } else {
      MQTT::Ping ping;
      ping.send(_client);
      lastOutActivity = t;
      lastInActivity = t;
      pingOutstanding = true;
    }
  }
  if (_client.available()) {
    MQTT::Message *msg = MQTT::readPacket(_client);
    if (msg != NULL) {
      lastInActivity = t;
      switch (msg->type()) {
      case MQTTPUBLISH:
	if (_callback) {
	  auto pub = (MQTT::Publish*)msg;
	  if (pub->qos()) {
	    _callback(*pub);
                    
	    MQTT::PublishAck puback(pub->packet_id());
	    puback.send(_client);
	    lastOutActivity = t;

	  } else {
	    _callback(*pub);
	  }
	}
	free(msg);
	return true;

      case MQTTPUBREC:
	{
	  auto pubrec = (MQTT::PublishRec*)msg;
	  MQTT::PublishRel pubrel(pubrec->packet_id());
	  pubrel.send(_client);
	  lastOutActivity = t;
	}
	free(msg);
	return true;

      case MQTTPUBREL:
	{
	  auto pubrel = (MQTT::PublishRel*)msg;
	  MQTT::PublishComp pubcomp(pubrel->packet_id());
	  pubcomp.send(_client);
	  lastOutActivity = t;
	}
	free(msg);
	return true;

      case MQTTPINGREQ:
	{
	  MQTT::PingResp pr;
	  pr.send(_client);
	  lastOutActivity = t;
	}
	free(msg);
	return true;

      case MQTTPINGRESP:
	pingOutstanding = false;
	free(msg);
	return true;
      }
      free(msg);
    }
  }
  return true;
}

bool PubSubClient::publish(String topic, String payload) {
  if (!connected())
    return false;

  MQTT::Publish pub(topic, payload);
  return pub.send(_client);
}

bool PubSubClient::publish(String topic, const uint8_t* payload, unsigned int plength, bool retained) {
  if (!connected())
    return false;

  MQTT::Publish pub(topic, const_cast<uint8_t*>(payload), plength);
  pub.set_retain(retained);
  return publish(pub);
}

bool PubSubClient::publish(MQTT::Publish &pub) {
  if (!connected())
    return false;

  return pub.send(_client);
}

bool PubSubClient::subscribe(String topic, uint8_t qos) {
  if (!connected())
    return false;

  if (qos > 2)
    return false;

  MQTT::Subscribe sub(next_packet_id(), topic, qos);
  return sub.send(_client);
}

bool PubSubClient::subscribe(MQTT::Subscribe &sub) {
  if (!connected())
    return false;

  return sub.send(_client);
}

bool PubSubClient::unsubscribe(String topic) {
  if (!connected())
    return false;

  MQTT::Unsubscribe unsub(next_packet_id(), topic);
  return unsub.send(_client);
}

bool PubSubClient::unsubscribe(MQTT::Unsubscribe &unsub) {
  if (!connected())
    return false;

  return unsub.send(_client);
}

void PubSubClient::disconnect() {
   MQTT::Disconnect discon;
   discon.send(_client);
   _client.stop();
   lastInActivity = lastOutActivity = millis();
}

bool PubSubClient::connected() {
   bool rc = _client.connected();
   if (!rc)
     _client.stop();

   return rc;
}
