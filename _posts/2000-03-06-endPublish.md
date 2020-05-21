---
tag: api
type: function
name: endPublish
returns:
  type: boolean
  values:
    - value: 'false'
      description: publish failed, either connection lost or message too large
    - value: 'true'
      description: publish succeeded
---

Finishing sending a message that was started with a call to <code>beginPublish</code>.