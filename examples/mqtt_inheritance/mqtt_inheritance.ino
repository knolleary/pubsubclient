/*
  MQTT class inheritance example

  This sketch demonstrates how to create a custom class inheriting from
  PubSubClient.

  This could be useful in order to create a high-level object with all
  methods and parameters to interface with the MQTT broker.  That way, all the
  MQTT logic is defined within a separate class, allowing a clear code
  separation.

  For real projects, the MqttBackend class should be defined in separates .h and
  .cpp files.

  This example is based on the mqtt_esp8266 example.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.
const char* ssid = "........";
const char* password = "........";
const char* mqtt_server_ip = "192.168.0.1";
const int   mqtt_server_port = 1883;

WiFiClient espClient;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

#define MAX_SIZE_MY_ID 16

class MqttBackend : public PubSubClient {
  public:
    MqttBackend(Client& wifiClient) : PubSubClient(wifiClient) {};
    // to be called within Arduino setup() global function
    void setup(const char *serverIP = "192.168.1.1", int port = 1883, const char *myId = "ESP-SENSOR");
    bool reconnect();

  private:
    IPAddress _serverIPAddress;
    int _serverPort;
    char _id[MAX_SIZE_MY_ID];   // hostname of the current device (i.e. ESP8266)
  protected:
    virtual void onCallback(char* topic, byte* payload, unsigned int length);
};

void MqttBackend::setup(const char* serverIP, int port, const char* myId) {
	this->_serverIPAddress.fromString(serverIP);
	this->_serverPort = port;
	setServer(_serverIPAddress, _serverPort);
	strncpy(_id, myId, MAX_SIZE_MY_ID);

	reconnect();
}

bool MqttBackend::reconnect() {
  // Loop until we're reconnected
  while (!this->connected()) {
    if (!this->connect(_id)) {
      Serial.print("failed, rc=");
      Serial.print(this->state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      continue;
    }
    Serial.println("Connection to MQTT broker done :-)");
    // Once connected, publish an announcement...
    publish("log", "I'm born. Hello world!");
    // ... and resubscribe
    subscribe("inTopic");
  }
	return true;
}

void MqttBackend::onCallback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s] ", topic);
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}


MqttBackend mqttClient(espClient);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  mqttClient.setup(mqtt_server_ip, mqtt_server_port, "ESP8266-TEST");
}

void loop() {

  mqttClient.reconnect();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%d", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    mqttClient.publish("outTopic", msg);
  }
}
