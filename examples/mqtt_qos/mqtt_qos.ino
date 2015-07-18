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

WiFiClient wclient;
PubSubClient client(wclient, server);

void setup() {
  // Setup console
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);

    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
    Serial.println("WiFi connected");
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!client.connected()) {
      if (client.connect("arduinoClient")) {
	client.set_callback(callback);
	client.publish("outTopic", "hello world qos=0");	// Simple publish with qos=0

	client.publish(MQTT::Publish("outTopic", "hello world qos=1")
		       .set_qos(1));

	client.publish(MQTT::Publish("outTopic", "hello world qos=2")
		       .set_qos(2));
      }
    }

    if (client.connected())
      client.loop();
  }
}

