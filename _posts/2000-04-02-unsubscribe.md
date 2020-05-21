---
tag: api
type: function
name: unsubscribe
params:
- name: topic
  description: the topic to unsubscribe from
  type: const char[]
returns:
  type: boolean
  values:
    - value: 'false'
      description: sending the unsubscribe failed, either connection lost or message too large
    - value: 'true'
      description: sending the unsubscribe succeeded
---

Unsubscribes from the specified topic.
