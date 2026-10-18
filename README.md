# Arduino Client for MQTT

This library provides a client for doing simple publish/subscribe messaging with
a server that supports MQTT.

## Notes about this fork

This fork of [knolleary/pubsubclient](https://github.com/knolleary/pubsubclient) is based on the last release [v2.8](https://github.com/knolleary/pubsubclient/releases/tag/v2.8) on May 20, 2020. There was a maintenance approach in [#1045](https://github.com/knolleary/pubsubclient/issues/1045), but [Nick](https://github.com/knolleary) meanwhile stated that [PubSubClient is no longer maintained](https://github.com/knolleary/pubsubclient/blob/master/README.md).

I evaluated several other MQTT libraries, but none met my requirements for both resource usage and maintenance:

 - https://github.com/256dpi/arduino-mqtt
 - https://github.com/hideakitai/MQTTPubSubClient
 - https://github.com/bertmelis/espMqttClient
 - https://github.com/arduino-libraries/ArduinoMqttClient
 - https://github.com/thingsboard/pubsubclient

With no suitable alternative, I decided to manually merge the most important PR changes and release them as a new major version. I renamed the library __PubSubClient3__ to distinguish from the original while keeping a similar name.

Contributions are welcome.

## Examples

The library comes with a number of example sketches. See _File > Examples > PubSubClient3_
within the Arduino application.

Full API documentation is available here: https://hmueller01.github.io/pubsubclient3/api

## Limitations

 - The client is based on the [MQTT Version 3.1.1 specification](https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html) with some limitations.
 - It can publish at QoS 0 and since [v3.2.0](https://github.com/hmueller01/pubsubclient3/releases/tag/v3.2.0) also at QoS 1 or 2.

   **WARNING:** No retransmission is supported to keep the library as much memory friendly as possible.

   Note: Without retransmission support, the publish QoS is only meaningful when the broker sends your
   message to a subscriber, supposing that the subscriber subscribes with a QoS greater then or equal to
   the publish QoS. Consider that MQTT runs over TCP, so retransmission isn't really required in most cases, especially when publishing to the broker.
 - It can subscribe at QoS 0 or QoS 1.
 - The maximum message size, including header, is **256 bytes** by default. This
   is configurable via `MQTT_MAX_PACKET_SIZE` in `PubSubClient.h` or can be changed
   by calling `PubSubClient::setBufferSize(size)`.
 - The keepalive interval is set to 15 seconds by default. This is configurable
   via `MQTT_KEEPALIVE` in `PubSubClient.h` or can be changed by calling
   `PubSubClient::setKeepAlive(keepAlive)`.
 - The client uses MQTT 3.1.1 by default. It can be changed to use MQTT 3.1 by
   changing value of `MQTT_VERSION` in `PubSubClient.h`.
 - Since [v3.3.0](https://github.com/hmueller01/pubsubclient3/releases/tag/v3.3.0) it can publish and subscribe to `PROGMEM` or `__FlashStringHelper` topics.
   Details see [mqtt_progmem](https://github.com/hmueller01/pubsubclient3/blob/aae84e4d1aa65e752e19e30239b5796b4fe2705b/examples/mqtt_progmem/src/mqtt_progmem.cpp#L39-L48) example.
   But if you like to publish `PROGMEM` topics you have to use
   ```c
   const char TOPIC[] PROGMEM = "test";
   const char HELLO_WORLD[] PROGMEM = "hello world";
   client.beginPublish_P(TOPIC, strlen_P(HELLO_WORLD), MQTT_QOS0, false);
   client.write_P(HELLO_WORLD);
   client.endPublish();
   ```
   as `client.publish_P(...)` is already used for `PROGMEM` payloads.

## Compatible Hardware

The library uses the Arduino Ethernet Client api for interacting with the
underlying network hardware. This means it Just Works with a growing number of
boards and shields, including:

 - Arduino Ethernet
 - Arduino Ethernet Shield
 - Arduino YUN – use the included `YunClient` in place of `EthernetClient`, and
   be sure to do a `Bridge.begin()` first
 - Arduino WiFi Shield - if you want to send packets > 90 bytes with this shield,
   enable the `MQTT_MAX_TRANSFER_SIZE` define in `PubSubClient.h`.
 - Sparkfun WiFly Shield – [library](https://github.com/dpslwk/WiFly)
 - TI CC3000 WiFi - [library](https://github.com/sparkfun/SFE_CC3000_Library)
 - Intel Galileo/Edison
 - ESP8266
 - ESP32

The library cannot currently be used with hardware based on the ENC28J60 chip –
such as the Nanode or the Nuelectronics Ethernet Shield. For those, there is an
[alternative library](https://github.com/njh/NanodeMQTT) available.

## Usage

If using the [Arduino IDE](https://docs.arduino.cc/software/ide/) open the library manager and search for __PubSubClient3__.

In case of [PlatformIO](https://platformio.org/) you find detailed [installation information here](https://registry.platformio.org/libraries/hmueller01/PubSubClient3/installation).

## Changelog

See [CHANGELOG.md](CHANGELOG.md)

## License

This code is released under the [MIT License](LICENSE.txt).
