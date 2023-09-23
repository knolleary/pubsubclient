---
layout: default
title: Arduino Client for MQTT
---

This library provides a client for doing simple publish/subscribe messaging with a server that supports MQTT

For more information about MQTT, visit [MQTT.org](http://mqtt.org).

## Download

The latest version of the library can be downloaded from [GitHub](https://github.com/knolleary/pubsubclient/releases/latest).

## Documentation

The library comes with a number of example sketches. See `File > Examples > PubSubClient` within the Arduino application.

Full <a href="api.html">[API Documentation](/api) is available.

## Author

 - Nick O'Leary - [@knolleary](https://twitter.com/knolleary)

## License

This library is released under the [MIT License](http://www.opensource.org/licenses/mit-license.php).


## Change History

 The complete change history is available on [GitHub](https://github.com/knolleary/pubsubclient/commits/master).

#### Latest version: 2.8 <small> - released 2020-05-20</small>

 - Add `setBufferSize()` to override `MQTT_MAX_PACKET_SIZE`
 - Add `setKeepAlive()` to override `MQTT_KEEPALIVE`
 - Add `setSocketTimeout()` to override `MQTT_SOCKET_TIMEOUT`
 - Added check to prevent subscribe/unsubscribe to empty topics
 - Declare wifi mode prior to connect in ESP example
 - Use `strnlen` to avoid overruns
 - Support pre-connected Client objects
