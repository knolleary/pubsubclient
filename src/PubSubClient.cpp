/*
 PubSubClient.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include <string.h>

PubSubClient::PubSubClient(Client& c) :
  _callback(NULL),
  _client(&c),
  _max_retries(10)
{}

PubSubClient::PubSubClient(Client& c, IPAddress &ip, uint16_t port) :
  _callback(NULL),
  _client(&c),
  _max_retries(10),
  server_ip(ip),
  server_port(port)
{}

PubSubClient::PubSubClient(Client& c, String hostname, uint16_t port) :
  _callback(NULL),
  _client(&c),
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

MQTT::Message* PubSubClient::_recv_message(void) {
  MQTT::Message *msg = MQTT::readPacket(*_client);
  if (msg != NULL)
    lastInActivity = millis();
  return msg;
}

void PubSubClient::_process_message(MQTT::Message* msg) {
  switch (msg->type()) {
  case MQTT::PUBLISH:
    {
      MQTT::Publish *pub = static_cast<MQTT::Publish*>(msg);	// RTTI is disabled on embedded, so no dynamic_cast<>()

      if (_callback)
	_callback(*pub);

      if (pub->qos() == 1) {
	MQTT::PublishAck puback(pub->packet_id());
	puback.send(*_client);
	lastOutActivity = millis();

      } else if (pub->qos() == 2) {
	uint8_t retries = 0;

	{
	  MQTT::PublishRec pubrec(pub->packet_id());
	  if (!_send_reliably(&pubrec))
	    return;
	}

	{
	  MQTT::PublishComp pubcomp(pub->packet_id());
	  pubcomp.send(*_client);
	  lastOutActivity = millis();
	}
      }
    }
    break;

  case MQTT::PINGREQ:
    {
      MQTT::PingResp pr;
      pr.send(*_client);
      lastOutActivity = millis();
    }
    break;

  case MQTT::PINGRESP:
    pingOutstanding = false;
  }
}

bool PubSubClient::_wait_for(MQTT::message_type match_type, uint16_t match_pid) {
  while (!_client->available()) {
    if (millis() - lastInActivity > keepalive * 1000UL)
      return false;
    yield();
  }

  while (millis() < lastInActivity + (keepalive * 1000)) {
    // Read the packet and check it
    MQTT::Message *msg = _recv_message();
    if (msg != NULL) {
      if (msg->type() == match_type) {
	uint8_t pid = msg->packet_id();
	delete msg;
	if (match_pid)
	  return pid == match_pid;
	return true;
      }

      _process_message(msg);
      delete msg;
    }

    yield();
  }

  return false;
}

bool PubSubClient::_send_reliably(MQTT::Message* msg) {
  MQTT::message_type r_type = msg->response_type();

  if (msg->need_packet_id())
    msg->set_packet_id(_next_packet_id());
  uint16_t pid = msg->packet_id();

  uint8_t retries = 0;
 send:
  msg->send(*_client);
  lastOutActivity = millis();

  if (r_type == MQTT::None)
    return true;

  if (!_wait_for(r_type, pid)) {
    if (retries < _max_retries) {
      retries++;
      goto send;
    }
    return false;
  }
  return true;
}

bool PubSubClient::connect(String id) {
  return connect(id, "", 0, false, "");
}

bool PubSubClient::connect(String id, String willTopic, uint8_t willQos, bool willRetain, String willMessage) {
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
    result = _client->connect(server_hostname.c_str(), server_port);
  else
    result = _client->connect(server_ip, server_port);

  if (!result) {
    _client->stop();
    return false;
  }

  pingOutstanding = false;
  nextMsgId = 1;		// Init the next packet id
  lastInActivity = millis();	// Init this so that _wait_for() doesn't think we've already timed-out
  keepalive = conn.keepalive();	// Store the keepalive period from this connection

  bool ret = _send_reliably(&conn);
  if (!ret)
    _client->stop();

  return ret;
}

bool PubSubClient::loop() {
  if (!connected())
    return false;

  unsigned long t = millis();
  if ((t - lastInActivity > keepalive * 1000UL) || (t - lastOutActivity > keepalive * 1000UL)) {
    if (pingOutstanding) {
      _client->stop();
      return false;
    } else {
      MQTT::Ping ping;
      ping.send(*_client);
      lastInActivity = lastOutActivity = t;
      pingOutstanding = true;
    }
  }
  if (_client->available()) {
    // Read the packet and check it
    MQTT::Message *msg = _recv_message();
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

bool PubSubClient::publish(String topic, const uint8_t* payload, uint32_t plength, bool retained) {
  if (!connected())
    return false;

  MQTT::Publish pub(topic, const_cast<uint8_t*>(payload), plength);
  pub.set_retain(retained);
  return publish(pub);
}

bool PubSubClient::publish_P(String topic, PGM_P payload, uint32_t plength, bool retained) {
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
    pub.send(*_client);
    lastOutActivity = millis();
    break;

  case 1:
    if (!_send_reliably(&pub))
      return false;
    break;

  case 2:
    {
      if (!_send_reliably(&pub))
	return false;

      MQTT::PublishRel pubrel(pub.packet_id());
      if (!_send_reliably(&pubrel))
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

  MQTT::Subscribe sub(topic, qos);
  return subscribe(sub);
}

bool PubSubClient::subscribe(MQTT::Subscribe &sub) {
  if (!connected())
    return false;

  if (!_send_reliably(&sub))
    return false;

  return true;
}

bool PubSubClient::unsubscribe(String topic) {
  if (!connected())
    return false;

  MQTT::Unsubscribe unsub(topic);
  return unsubscribe(unsub);
}

bool PubSubClient::unsubscribe(MQTT::Unsubscribe &unsub) {
  if (!connected())
    return false;

  if (!_send_reliably(&unsub))
    return false;

  return true;
}

void PubSubClient::disconnect() {
   if (!connected())
     return;

   MQTT::Disconnect discon;
   discon.send(*_client);
   _client->stop();
   lastInActivity = lastOutActivity = millis();
}

bool PubSubClient::connected() {
   bool rc = _client->connected();
   if (!rc)
     _client->stop();

   return rc;
}
