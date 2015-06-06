/*
 PubSubClient.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include <string.h>

PubSubClient::PubSubClient() :
  _callback(NULL),
  _max_retries(10)
{}

PubSubClient::PubSubClient(IPAddress &ip, uint16_t port) :
  _callback(NULL),
  _max_retries(10),
  server_ip(ip),
  server_port(port)
{}

PubSubClient::PubSubClient(String hostname, uint16_t port) :
  _callback(NULL),
  _max_retries(10),
  server_port(port),
  server_hostname(hostname)
{}

PubSubClient& PubSubClient::set_server(IPAddress &ip, uint16_t port) {
  server_hostname = "";
  server_ip = ip;
  server_port = port;
  return *this;
}

PubSubClient& PubSubClient::set_server(String hostname, uint16_t port) {
  server_hostname = hostname;
  server_port = port;
  return *this;
}

bool PubSubClient::_process_message(MQTT::Message* msg, uint8_t match_type, uint16_t match_pid) {
  lastInActivity = millis();
  if (msg->type() == match_type) {
    if (match_pid)
      return msg->packet_id() == match_pid;
    return true;
  }

  switch (msg->type()) {
  case MQTTPUBLISH:
    {
      auto pub = static_cast<MQTT::Publish*>(msg);	// RTTI is disabled on ESP8266, so no dynamic_cast<>()

      if (_callback)
	_callback(*pub);

      if (pub->qos() == 1) {
	MQTT::PublishAck puback(pub->packet_id());
	puback.send(_client);
	lastOutActivity = millis();

      } else if (pub->qos() == 2) {
	uint8_t retries = 0;

	{
	  MQTT::PublishRec pubrec(pub->packet_id());
	  if (!send_reliably(&pubrec))
	    return false;
	}

	{
	  MQTT::PublishComp pubcomp(pub->packet_id());
	  pubcomp.send(_client);
	  lastOutActivity = millis();
	}
      }
    }
    break;

  case MQTTPINGREQ:
    {
      MQTT::PingResp pr;
      pr.send(_client);
      lastOutActivity = millis();
    }
    break;

  case MQTTPINGRESP:
    pingOutstanding = false;
  }

  return false;
}

bool PubSubClient::wait_for(uint8_t match_type, uint16_t match_pid) {
  while (!_client.available()) {
    if (millis() - lastInActivity > keepalive * 1000UL)
      return false;
    delayMicroseconds(100);
  }

  while (millis() < lastInActivity + (keepalive * 1000)) {
    // Read the packet and check it
    MQTT::Message *msg = MQTT::readPacket(_client);
    if (msg != NULL) {
      bool ret = _process_message(msg, match_type, match_pid);
      delete msg;
      if (ret)
	return true;
    }

    delayMicroseconds(100);
  }

  return false;
}

bool PubSubClient::send_reliably(MQTT::Message* msg) {
  uint8_t retries = 0;
 send:
  msg->send(_client);
  lastOutActivity = millis();

  if (msg->response_type() == 0)
    return true;

  if (!wait_for(msg->response_type(), msg->packet_id())) {
    if (retries < _max_retries) {
      retries++;
      goto send;
    }
    return false;
  }
  return true;
}

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

  if (!result) {
    _client.stop();
    return false;
  }

  nextMsgId = 1;		// Init the next packet id
  lastInActivity = millis();	// Init this so that wait_for() doesn't think we've already timed-out
  keepalive = conn.keepalive();	// Store the keepalive period from this connection

  bool ret = send_reliably(&conn);
  if (!ret)
    _client.stop();

  return ret;
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
      lastInActivity = lastOutActivity = t;
      pingOutstanding = true;
    }
  }
  if (_client.available()) {
    // Read the packet and check it
    MQTT::Message *msg = MQTT::readPacket(_client);
    if (msg != NULL) {
      _process_message(msg);
      delete msg;
    }
  }
  return true;
}

bool PubSubClient::publish(String topic, String payload) {
  if (!connected())
    return false;

  MQTT::Publish pub(topic, payload);
  return publish(pub);
}

bool PubSubClient::publish(String topic, const uint8_t* payload, unsigned int plength, bool retained) {
  if (!connected())
    return false;

  MQTT::Publish pub(topic, const_cast<uint8_t*>(payload), plength);
  pub.set_retain(retained);
  return publish(pub);
}

bool PubSubClient::publish_P(String topic, PGM_P payload, unsigned int plength, bool retained) {
  if (!connected())
    return false;

  MQTT::Publish pub = MQTT::Publish_P(topic, payload, plength);
  pub.set_retain(retained);
  return publish(pub);
}

bool PubSubClient::publish(MQTT::Publish &pub) {
  if (!connected())
    return false;

  switch (pub.qos()) {
  case 0:
    pub.send(_client);
    lastOutActivity = millis();
    break;

  case 1:
    if (!send_reliably(&pub))
      return false;
    break;

  case 2:
    {
      if (!send_reliably(&pub))
	return false;

      MQTT::PublishRel pubrel(pub.packet_id());
      if (!send_reliably(&pubrel))
	return false;
    }
    break;
  }
  return true;
}

bool PubSubClient::subscribe(String topic, uint8_t qos) {
  if (!connected())
    return false;

  if (qos > 2)
    return false;

  MQTT::Subscribe sub(next_packet_id(), topic, qos);
  return subscribe(sub);
}

bool PubSubClient::subscribe(MQTT::Subscribe &sub) {
  if (!connected())
    return false;

  if (!send_reliably(&sub))
    return false;

  return true;
}

bool PubSubClient::unsubscribe(String topic) {
  if (!connected())
    return false;

  MQTT::Unsubscribe unsub(next_packet_id(), topic);
  return unsubscribe(unsub);
}

bool PubSubClient::unsubscribe(MQTT::Unsubscribe &unsub) {
  if (!connected())
    return false;

  if (!send_reliably(&unsub))
    return false;

  return true;
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
