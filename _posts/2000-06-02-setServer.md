---
tag: api
type: function
name: setServer
params:
  - name: server
    description: the address of the server
    type: IPAddress, uint8_t[] or const char[]
  - name: port
    description: the port to connect to
    type: int

returns:
  type: PubSubClient*
  values:
    - value: PubSubClient*
      description: the client instance, allowing the function to be chained
---

Sets the server details.