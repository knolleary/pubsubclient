---
tag: api
type: function
name: setBufferSize
params:
  - name: size
    description: the size, in bytes, for the internal buffer
    type: uint16_t
returns:
  type: boolean
  values:
    - value: 'false'
      description: the buffer could not be resized
    - value: 'true'
      description: the buffer was resized
---

Sets the size, in bytes, of the internal send/receive buffer. This must be large
enough to contain the full MQTT packet. When sending or receiving messages,
the packet will contain the full topic string, the payload data and a small number
of header bytes.

By default, it is set to `256` bytes - as defined by the `MQTT_MAX_MESSAGE_SIZE`
constant in `PubSubClient.h`.

*Note* : `setBufferSize` returns a boolean flag to indicate whether it was able
to reallocate the memory to change the buffer size. This means, unlike the other
`setXYZ` functions that return a reference to the client, this function cannot be
chained with those functions.
