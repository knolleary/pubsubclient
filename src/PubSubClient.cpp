/*
 PubSubClient.cpp - A simple client for MQTT.
  Nicholas O'Leary
  http://knolleary.net
*/

#include "PubSubClient.h"
#include <string.h>

PubSubClient::PubSubClient(Client& c) :
  _callback(nullptr),
  _client(c),
  _parser(c),
  _max_retries(10),
  isSubAckFound(false)
{}

PubSubClient::PubSubClient(Client& c, IPAddress &ip, uint16_t port) :
  _callback(nullptr),
  _client(c),
  _parser(c),
  _max_retries(10),
  isSubAckFound(false),
  server_ip(ip),
  server_port(port)
{}

PubSubClient::PubSubClient(Client& c, String hostname, uint16_t port) :
  _callback(nullptr),
  _client(c),
  _parser(c),
  _max_retries(10),
  isSubAckFound(false),
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
  MQTT::Message *msg = _parser.parse();
  if (msg != nullptr)
    lastInActivity = millis();
  return msg;
}

bool PubSubClient::_send_message(MQTT::Message& msg) {
  if (msg.need_packet_id())
    msg.set_packet_id(_next_packet_id());

  uint8_t retries = 0;
 send:
  if (!msg.send(_client)) {
    if (retries < _max_retries) {
      retries++;
      goto send;
    }
    return false;
  }
  lastOutActivity = millis();

  return true;
}

MQTT::Message* PubSubClient::_send_message_with_response(MQTT::Message& msg) {
  if (msg.need_packet_id())
    msg.set_packet_id(_next_packet_id());

  uint8_t retries = 0;
 send:
  if (!msg.send(_client)) {
    if (retries < _max_retries) {
      retries++;
      goto send;
    }
    return nullptr;
  }
  lastOutActivity = millis();

  MQTT::Message *response = _wait_for(msg.response_type(), msg.packet_id());
  if (response == nullptr) {
    if (retries < _max_retries) {
      retries++;
      goto send;
    }
    return nullptr;
  }

  return response;
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
	_send_message(puback);

      } else if (pub->qos() == 2) {

	{
	  MQTT::PublishRec pubrec(pub->packet_id());
	  MQTT::Message *response = _send_message_with_response(pubrec);
	  if (response == nullptr)
	    return;
	  delete response;
	}

	{
	  MQTT::PublishComp pubcomp(pub->packet_id());
	  _send_message(pubcomp);
	}
      }
    }
    break;

  case MQTT::PINGREQ:
    {
      MQTT::PingResp pr;
      _send_message(pr);
    }
    break;

  case MQTT::PINGRESP:
    pingOutstanding = false;
  }
}

MQTT::Message* PubSubClient::_wait_for(MQTT::message_type wait_type, uint16_t wait_pid) {
  while (!_client.available()) {
    if (millis() - lastInActivity > keepalive * 1000UL)
      return nullptr;
    yield();
  }

  while (millis() < lastInActivity + (keepalive * 1000)) {
    // Read the packet and check it
    MQTT::Message *msg = _recv_message();
    if (msg != nullptr) {
      if (msg->type() == wait_type) {
	if ((wait_pid > 0) && (msg->packet_id() != wait_pid)) {
	  delete msg;
	  return nullptr;
	}
	return msg;
      } else if (msg->type() == MQTT::SUBACK) { // if the current message is not the one we want
        // Signal that we found a SUBACK message
        isSubAckFound = true;
      }

      _process_message(msg);

      // After having proceeded new incoming packets, we check if our response as not already been processed
      if ((wait_type == MQTT::SUBACK) && isSubAckFound) {
        isSubAckFound = false;
        // Return false will cause a resend of a SUBSCRIBE message (and so a new chance to get a SUBACK)
        return nullptr;
      }

      delete msg;
    }

    yield();
  }

  return nullptr;
}

bool PubSubClient::connect(String id) {
  MQTT::Connect conn(id);
  return connect(conn);
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
    result = _client.connect(server_hostname.c_str(), server_port);
  else
    result = _client.connect(server_ip, server_port);

  if (!result) {
    _client.stop();
    return false;
  }

  pingOutstanding = false;
  nextMsgId = 1;		// Init the next packet id
  lastInActivity = millis();	// Init this so that _wait_for() doesn't think we've already timed-out
  keepalive = conn.keepalive();	// Store the keepalive period from this connection

  MQTT::Message *response = _send_message_with_response(conn);
  if (response == nullptr) {
    _client.stop();
    return false;
  }

  bool ret = true;
  if (response->type() == MQTT::CONNACK) {
    MQTT::ConnectAck *ack = static_cast<MQTT::ConnectAck*>(response);
    if (ack->rc() > 0) {
      _client.stop();
      ret = false;
    }
  }
  delete response;

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
      if (!_send_message(ping))
	return false;

      lastInActivity = lastOutActivity;
      pingOutstanding = true;
    }
  }
  if (_client.available()) {
    // Read the packet and check it
    MQTT::Message *msg = _recv_message();
    if (msg != nullptr) {
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

bool PubSubClient::publish(String topic, MQTT::payload_callback_t pcb, uint32_t length, bool retained) {
  if (!connected())
    return false;

  MQTT::Publish pub(topic, pcb, length);
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

  MQTT::Message *response;
  switch (pub.qos()) {
  case 0:
    return _send_message(pub);

  case 1:
    response = _send_message_with_response(pub);
    if (response == nullptr)
      return false;

    delete response;
    return true;

  case 2:
    {
      response =_send_message_with_response(pub);
      if (response == nullptr)
	return false;

      delete response;

      MQTT::PublishRel pubrel(pub.packet_id());
      response = _send_message_with_response(pubrel);
      if (response == nullptr)
	return false;

      delete response;
      return true;
    }
  }
  return false;
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

  MQTT::Message *response = _send_message_with_response(sub);
  if (response == nullptr)
    return false;

  delete response;
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

  MQTT::Message *response = _send_message_with_response(unsub);
  if (response == nullptr)
    return false;

  delete response;
  return true;
}

void PubSubClient::disconnect() {
   if (!connected())
     return;

   MQTT::Disconnect discon;
   if (_send_message(discon))
     lastInActivity = lastOutActivity;
   _client.stop();
}

bool PubSubClient::connected() {
   bool rc = _client.connected();
   if (!rc)
     _client.stop();

   return rc;
}
