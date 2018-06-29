/*
  Basic ESP32 MQTT example
  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP32 board/library.
  It connects to an MQTT server then:
  - publishes "connected to MQTT" to the topic "outTopic"
  - subscribes to the topic "inTopic", printing out messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the "ON" payload is send to the topic "inTopic" , LED will be turned on, and acknowledgement will be send to Topic "outTopic"
  - If the "OFF" payload is send to the topic "inTopic" , LED will be turned OFF, and acknowledgement will be send to Topic "outTopic"
  It will reconnect to the server if the connection is lost using a
  reconnect function.
*/

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#if defined(ESP8266)
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif
#include <PubSubClient.h>

const char* mqtt_server = ".........."; //mqtt server
const char* ssid = "........";
const char* password = "........";

WiFiClient espClient;
PubSubClient client(espClient); //lib required for mqtt

int LED = 02;
void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  WiFi.begin(ssid, password);
  Serial.println("connected");
  client.setServer(mqtt_server, 1883);//connecting to mqtt server
  client.setCallback(callback);
  //delay(5000);
  connectmqtt();
}

void callback(char* topic, byte* payload, unsigned int length) {   //callback includes topic and payload ( from which (topic) the payload is comming)
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  if ((char)payload[0] == 'O' && (char)payload[1] == 'N') //on
  {
    digitalWrite(LED, HIGH);
    Serial.println("on");
    client.publish("outTopic", "LED turned ON");
  }
  else if ((char)payload[0] == 'O' && (char)payload[1] == 'F' && (char)payload[2] == 'F') //off
  {
    digitalWrite(LED, LOW);
    Serial.println(" off");
    client.publish("outTopic", "LED turned OFF");
  }
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32_clientID")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "Nodemcu connected to MQTT");
      // ... and resubscribe
      client.subscribe("inTopic");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();
}


void connectmqtt()
{
  client.connect("ESP32_clientID");  // nodemcu will connect to mqtt broker with clientID
  {
    Serial.println("connected to MQTT");
    // Once connected, publish an announcement...

    // ... and resubscribe
    client.subscribe("inTopic"); //topic=Demo
    client.publish("outTopic",  "connected to MQTT");

    if (!client.connected())
    {
      reconnect();
    }
  }
}
