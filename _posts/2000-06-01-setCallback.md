---
tag: api
type: function
name: setCallback
params:
  - name: callback
    description: a pointer to a message callback function called when a message arrives for a subscription created by this client.
    type: function*
returns:
  type: PubSubClient*
  values:
    - value: PubSubClient*
      description: the client instance, allowing the function to be chained
---

Sets the <a href="#callback">message callback function</a>.
