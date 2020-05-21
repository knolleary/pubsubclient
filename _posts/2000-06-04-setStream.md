---
tag: api
type: function
name: setStream
params:
  - name: stream
    description: a stream to write received messages to
    type: Stream
returns:
  type: PubSubClient*
  values:
    - value: PubSubClient*
      description: the client instance, allowing the function to be chained
---

Sets the stream to write received messages to.

