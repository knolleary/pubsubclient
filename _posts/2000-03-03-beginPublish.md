---
tag: api
type: function
name: beginPublish
params:
- name: topic
  description: the topic to publish to
  type: const char[]
- name: length
  description: the length of the payload to be sent
  type: unsigned int
- name: retained
  description: whether the message should be retained
      <ul>
          <li>false - not retained</li>
          <li>true - retained</li>
      </ul>
  type: boolean
returns:
  type: boolean
  values:
    - value: 'false'
      description: publish failed, either connection lost or message too large
    - value: 'true'
      description: publish succeeded
---

Begins sending a publish message. The payload of the message is provided by one or more calls to <code>write</code> followed by a call to <code>endPublish</code>.
