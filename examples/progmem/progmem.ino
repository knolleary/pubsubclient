#include <Ethernet.h>
#include <PubSubClient.h>

const char test[] PROGMEM = "hello";

EthernetClient ethClient;
PubSubClient client(ethClient);

void setup() {
}

void loop() {
     client.publish_P("topic", test, false);
}