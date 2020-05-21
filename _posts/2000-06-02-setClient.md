---
tag: api
type: function
name: setClient
params:
  - name: client
    description: the network client to use, for example <code>WiFiClient</code>

returns:
  type: PubSubClient*
  values:
    - value: PubSubClient*
      description: the client instance, allowing the function to be chained
---

Sets the network client instance to use.