---
tag: api
type: constructor
name: PubSubClient
params:
  - name: client
    description: the network client to use, for example <code>WiFiClient</code>
---


Creates a partially initialised client instance.

Before it can be used, the server details must be configured:

```
EthernetClient ethClient;
PubSubClient client(ethClient);

void setup() {
    client.setServer("broker.example.com",1883);
    // client is now ready for use
}
```
