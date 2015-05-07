/*
 MQTT with QoS example

  - connects to an MQTT server
  - publishes "hello world" to the topic "outTopic" with a variety of QoS values

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char *ssid =	"xxxxxxxx";		// cannot be longer than 32 characters!
const char *pass =	"yyyyyyyy";		//

// Update these with values suitable for your network.
IPAddress server(172, 16, 0, 2);

void callback(const MQTT::Publish& pub) {
  // handle message arrived
}

PubSubClient client(server);

void setup()
{
  // Setup console
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();

  client.set_callback(callback);

  WiFi.begin(ssid, pass);

  int retries = 0;
  while ((WiFi.status() != WL_CONNECTED) && (retries < 10)) {
    retries++;
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
  }

  if (client.connect("arduinoClient")) {
    client.publish("outTopic", "hello world qos=0");	// Simple publish with qos=0

    client.publish(MQTT::Publish("outTopic", "hello world qos=1")
                   .set_qos(1, client.next_packet_id()));

    client.publish(MQTT::Publish("outTopic", "hello world qos=2")
                   .set_qos(2, client.next_packet_id()));
  }
}

void loop()
{
  client.loop();
}

