/*
PubSubClient_JSON.h - ArduinoJson support for PubSubClient
Copyright (C) 2016 Ian Tester

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

#pragma once

#include <ArduinoJson.h>

namespace MQTT {
  class ConnectJSON : public Connect {
  public:
    //! Set the "will" flag and attributes, with a JSON object "will" message
    Connect& set_will(String willTopic, const ArduinoJson::JsonDocument& willMessage, uint8_t willQos, bool willRetain) {
      _will_topic = willTopic;
      _will_qos = willQos;
      _will_retain = willRetain;

      if (_will_message != nullptr)
	delete [] _will_message;

      _will_message_len = measureJson(willMessage);
      _will_message = new uint8_t[_will_message_len + 1];
      if (_will_message != nullptr)
	serializeJson(willMessage, _will_message, _will_message_len + 1);

      return *this;
    }

  };

  class PublishJSON : public Publish {
  public:
    //! Publish a JSON object from the ArduinoJson library
    /*!
      \param topic Topic of the message
      \param payload Object of the message
    */
    PublishJSON(String topic, const ArduinoJson::JsonDocument& object) :
      Publish(topic, nullptr, measureJson(object))
    {
      _payload = new uint8_t[_payload_len + 1];
      if (_payload != nullptr)
	serializeJson(object, _payload, _payload_len + 1);
    }

  };

};
