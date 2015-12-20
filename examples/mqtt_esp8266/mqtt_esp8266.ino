/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "........";
const char* password = "........";

#define MQTTid              "mydevice"                   //id of this mqtt client
#define MQTTip              "broker.mqtt-dashboard.com"  //ip address or hostname of the mqtt broker
#define MQTTport            1883                         //port of the mqtt broker
#define MQTTuser            "user"                       //username of this mqtt client
#define MQTTpsw             "password"                   //password of this mqtt client
#define MQTTpubQos          2                            //qos of publish (see README)
#define MQTTsubQos          1                            //qos of subscribe

long lastMsg = 0;
char msg[50];
int value = 0;

boolean pendingDisconnect = false;
void mqttConnectedCb(); // on connect callback
void mqttDisconnectedCb(); // on disconnect callback
void mqttDataCb(char* topic, byte* payload, unsigned int length); // on new message callback

WiFiClient wclient;
PubSubClient client(MQTTip, MQTTport, mqttDataCb, wclient);

void mqttConnectedCb() {
  Serial.println("connected");
	
  // Once connected, publish an announcement...
  client.publish("outTopic", "hello world", MQTTpubQos, true); // true means retain
  // ... and resubscribe
  client.subscribe("inTopic", MQTTsubQos);

}

void mqttDisconnectedCb() {
  Serial.println("disconnected");
}

void mqttDataCb(char* topic, byte* payload, unsigned int length) {

  /*
  you can convert payload to a C string appending a null terminator to it;
  this is possible when the message (including protocol overhead) doesn't
  exceeds the MQTT_MAX_PACKET_SIZE defined in the library header.
  you can consider safe to do so when the length of topic plus the length of
  message doesn't exceeds 115 characters
  */
  char* message = (char *) payload;
  message[length] = 0;

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  // Switch on the LED if an 1 was received as first character
  if (message[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
}

void process_mqtt() {
  if (WiFi.status() == WL_CONNECTED) {
    if (client.connected()) {
      client.loop();
    } else {
	  // client id, client username, client password, last will topic, last will qos, last will retain, last will message
      if (client.connect(MQTTid, MQTTuser, MQTTpsw, MQTTid "/status", 2, true, "0")) {
          pendingDisconnect = false;
          mqttConnectedCb();
      }
    }
  } else {
    if (client.connected())
      client.disconnect();
  }
  if (!client.connected() && !pendingDisconnect) {
    pendingDisconnect = true;
    mqttDisconnectedCb();
  }
}

void loop() {

  process_mqtt();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld", value);
	
    if (client.connected()) {
        Serial.print("Publish message: ");
        Serial.println(msg);
        client.publish("outTopic", msg);
    }
  }
}