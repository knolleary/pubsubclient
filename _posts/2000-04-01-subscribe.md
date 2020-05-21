---
tag: api
type: function
name: subscribe
params:
- name: topic
  description: the topic to subscribe to
  type: const char[]
- name: qos
  optional: true
  description: the qos to subscribe at
  type: 'int: 0 or 1 only'
returns:
  type: boolean
  values:
    - value: 'false'
      description: sending the subscribe failed, either connection lost or message too large
    - value: 'true'
      description: sending the subscribe succeeded
---

Subscribes to messages published to the specified topic.

