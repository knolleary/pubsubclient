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
  class PublishJSON : public Publish {
  public:
    //! Publish a JSON object from the ArduinoJson library
    /*!
      \param topic Topic of the message
      \param payload Object of the message
    */
    PublishJSON(String topic, ArduinoJson::JsonObject& object) :
      Publish(topic, NULL, object.measureLength() + 1)
    {
      _payload = new uint8_t[_payload_len];
      if (_payload != NULL)
	object.printTo((char*)_payload, _payload_len);
    }

    //! Publish a JSON array from the ArduinoJson library
    /*!
      \param topic Topic of the message
      \param payload Array of the message
    */
    PublishJSON(String topic, ArduinoJson::JsonArray& array) :
      Publish(topic, NULL, array.measureLength() + 1)
    {
      _payload = new uint8_t[_payload_len];
      if (_payload != NULL)
	array.printTo((char*)_payload, _payload_len);
    }

  };

};
