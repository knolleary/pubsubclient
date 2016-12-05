/*
 MQTT "will" message example

  - connects to an MQTT server with a will message
  - publishes a message
  - waits a little bit
  - disconnects the socket *without* sending a disconnect packet

  You should see the will message published when we disconnect
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

void loop() {
  delay(1000);

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
    MQTT::Connect con("arduinoClient");
    con.set_will("test", "I am down.");
    // Or to set a binary message:
    // char msg[4] = { 0xde, 0xad, 0xbe, 0xef };
    // con.set_will("test", msg, 4);
    if (client.connect(con)) {
      client.publish("test", "I am up!");
      delay(1000);
      wclient.stop();
    } else
      Serial.println("MQTT connection failed.");

    delay(10000);
  }
}

