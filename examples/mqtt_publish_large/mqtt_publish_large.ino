/*
 Large publish message MQTT example

  - connects to an MQTT server
  - publishes a large string of text to the topic "outTopic"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char *ssid =	"xxxxxxxx";		// cannot be longer than 32 characters!
const char *pass =	"yyyyyyyy";		//

// Update these with values suitable for your network.
IPAddress server(172, 16, 0, 2);

WiFiClient wclient;
PubSubClient client(wclient, server);

void setup() {
  // Setup console
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
}

bool message(Client& client) {
  char buf[64];
  memset(buf, 32, 64);
  for (int l = 0; l < 1024; l++) {
    int len = sprintf(buf, "%d", l);
    buf[len] = 32;
    buf[63] = '\n';
    client.write((const uint8_t*)buf, 64);
  }
  Serial.println("Published large message.");
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
	client.publish("outTopic", message, 65536);
      }
    }

    if (client.connected())
      client.loop();
  }
}

