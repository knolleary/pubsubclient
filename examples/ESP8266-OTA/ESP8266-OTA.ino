/*
 * Test/demonstration of over-the-air update of an ESP8266 using MQTT.
 *
 * This sketch connects to an MQTT broker and subscribes to a specific topic
 * to receive OTA updates on.
 * Send the message using the mosquitto tool:
 $ mosquitto_pub -h 192.168.1.1 -t 'ota/192.168.1.13' -r -f /tmp/build*.tmp/sensor-node.cpp.bin
 *
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>

// Replace with your Wifi SSID and passphrase
const char* ssid = "xxxxxxxx";
const char* pass = "yyyyyyyy";

// Replace with the IP address of your MQTT server
IPAddress server_ip(192,168,1,1);

void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("Arduino MQTT OTA Test");

  Serial.printf("Sketch size: %u\n", ESP.getSketchSize());
  Serial.printf("Free size: %u\n", ESP.getFreeSketchSpace());
}

WiFiClient wclient;
PubSubClient client(wclient, server_ip);

void receive_ota(const MQTT::Publish& pub) {
  uint32_t startTime = millis();
  uint32_t size = pub.payload_len();
  if (size == 0)
    return;

  Serial.print("Receiving OTA of ");
  Serial.print(size);
  Serial.println(" bytes...");

  Serial.setDebugOutput(true);
  if (!Update.begin(size)) {
    Serial.println("Update Begin Error");
    return;
  }

  uint32_t total = 0;
  while (!Update.isFinished()) {
    uint32_t written = Update.write(*pub.payload_stream());
    if (written > 0) {
      total += written;
      Serial.print(total, DEC);
      Serial.println(" bytes");
    }
  }

  if (Update.end()) {
    Serial.println("Clearing retained message.");
    client.publish(MQTT::Publish(pub.topic(), "")
                   .set_retain());
    client.disconnect();

    Serial.printf("Update Success: %u\nRebooting...\n", millis() - startTime);
    ESP.restart();
  } else {
    Update.printError(Serial);
  }
  Serial.setDebugOutput(false);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println("...");
    WiFi.begin(ssid, pass);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;

    IPAddress local = WiFi.localIP();
    String ipaddr = String(local[0]) + "." + String(local[1]) + "." + String(local[2]) + "." + String(local[3]);
    Serial.print("IP address: ");
    Serial.println(ipaddr);

    client.connect(WiFi.macAddress());  // Give ourselves a unique client name
    client.set_callback(receive_ota);   // Register our callback for receiving OTA's

    String topic = "ota/" + ipaddr;
    Serial.print("Subscribing to topic ");
    Serial.println(topic);
    client.subscribe(topic);
  }
  client.loop();
}
