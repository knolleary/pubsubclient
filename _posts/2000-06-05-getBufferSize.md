---
tag: api
type: function
name: getBufferSize
returns:
  type: uint16_t
  values:
    - value: uint16_t
      description: the size of the internal buffer
---

Gets the current size of the internal buffer.

By default, it is set to `256` bytes - as defined by the `MQTT_MAX_MESSAGE_SIZE`
constant in `PubSubClient.h`.
