/*
 MQTT to serial example 
 
  - connects to an MQTT server
  - subscribes to the topic "inTopic"
  - Print all recived message to Serial port
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
byte server[] = { 172, 16, 0, 2 };
byte ip[]     = { 172, 16, 0, 100 };

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  Serial.println(“Callback”);
  Serial.print(“Topic:”);
  Serial.println(topic);
  Serial.print(“Length:”);
  Serial.println(length);
  Serial.print(“Payload:”);
  Serial.write(payload,length);
  Serial.println();
}

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

void setup()
{
  Ethernet.begin(mac, ip);
  if (client.connect("arduinoClient")) {
    client.subscribe("inTopic");
  }
}

void loop()
{
  client.loop();
}

