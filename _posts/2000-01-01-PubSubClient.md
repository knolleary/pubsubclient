---
tag: api
type: constructor
name: PubSubClient

---

Creates an uninitialised client instance.

Before it can be used, it must be configured with the property setters:

```
EthernetClient ethClient;
PubSubClient client;

void setup() {
    client.setClient(ethClient);
    client.setServer("broker.example.com",1883);
    // client is now configured for use
}
```
