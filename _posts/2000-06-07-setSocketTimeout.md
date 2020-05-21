---
tag: api
type: function
name: setSocketTimeout
params:
  - name: timeout
    description: the socket timeout, in seconds
    type: uint16_t
returns:
  type: PubSubClient*
  values:
    - value: PubSubClient*
      description: the client instance, allowing the function to be chained
---

Sets the socket timeout used by the client. This determines how long the client
will wait for incoming data when it expects data to arrive - for example, whilst
it is in the middle of reading an MQTT packet.

By default, it is set to `15` seconds - as defined by the `MQTT_SOCKET_TIMEOUT`
constant in `PubSubClient.h`.