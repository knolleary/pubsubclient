/*
 Example of using a Stream object to store the message payload

 Uses SRAM library: https://github.com/ennui2342/arduino-sram
 but could use any Stream based class such as SD

  - connects to an MQTT server
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic"
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SRAM.h>

const char *ssid =	"xxxxxxxx";		// cannot be longer than 32 characters!
const char *pass =	"yyyyyyyy";		//

// Update these with values suitable for your network.
IPAddress server(172, 16, 0, 2);

SRAM sram(4, SRAM_1024);

void callback(const MQTT::Publish& pub) {
  sram.seek(1);

  // do something with the message
  for (uint8_t i = 0; i < pub.payload_len(); i++) {
    uint8_t byte;
    if (pub.has_stream())
      pub.payload_stream()->read(&byte, 1);
    else
      byte = pub.payload()[i];
    sram.write(byte);
    Serial.write(byte);
  }

  if (pub.has_stream())
    pub.payload_stream()->stop();

  Serial.println();
}

WiFiClient wclient;
PubSubClient client(wclient, server);

void setup() {
  // Setup console
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();

  sram.begin();
  sram.seek(1);
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
	client.publish("outTopic","hello world");
	client.subscribe("inTopic");
      }
    }

    if (client.connected())
      client.loop();
  }
}

