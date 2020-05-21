---
tag: api
type: constructor
name: PubSubClient
params:
  - name: server
    description: the address of the server
    type: IPAddress, uint8_t[] or const char[]
  - name: port
    description: the port to connect to
    type: int
  - name: callback
    optional: true
    description: a pointer to a <a href="#callback">message callback function</a> called when a message arrives for a subscription created by this client
    type: function*
  - name: client
    description: the network client to use, for example <code>WiFiClient</code>
  - name: stream
    optional: true
    description: a stream to write received messages to
    type: Stream
---


Creates a fully configured client instance.
