---
tag: api
type: function
name: setKeepAlive
params:
  - name: keepAlive
    description: the keep alive interval, in seconds
    type: uint16_t
returns:
  type: PubSubClient*
  values:
    - value: PubSubClient*
      description: the client instance, allowing the function to be chained
---

Sets the keep alive interval used by the client. This value should only be changed
when the client is not connected.

By default, it is set to `15` seconds - as defined by the `MQTT_KEEPALIVE`
constant in `PubSubClient.h`.