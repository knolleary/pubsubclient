#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);
static volatile bool wifi_connected = false;
static volatile bool wifi_reconnect_in_progress = false;

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void setup_mqtt()
{
    mqtt_client.setServer("172.16.0.2", 1883);
    mqtt_client.setCallback(mqtt_callback);

    Serial.print("Attempting MQTT connection...");
    while (!mqtt_client.connected())
    {
        Serial.print(".");
        Serial.print(mqtt_client.state());

        if (mqtt_client.connect("arduinoClient"))
        {
            Serial.println("connected to MQTT");
            mqtt_client.publish("outTopic", "hello world");
            mqtt_client.subscribe("inTopic");
        }
        else
        {
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void setup_wifi();

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_CONNECTED:
        //enable sta ipv6 here
        WiFi.enableIpV6();
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        //both interfaces get the same event
        Serial.print("STA IPv6: ");
        Serial.println(WiFi.localIPv6());
        Serial.print("AP IPv6: ");
        Serial.println(WiFi.softAPIPv6());
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        wifi_connected = true;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        wifi_connected = false;
        Serial.println("WiFi Disconnected");
        setup_wifi();

        break;
    }
}

void setup_wifi()
{
    if (!wifi_reconnect_in_progress)
    {
        int local_count = 0;
        wifi_reconnect_in_progress = true;

        WiFi.onEvent(WiFiEvent);
        const char *ssid = "WiFiSSID";
        const char *password = "password";

        Serial.print("Connecting to WiFi SSID: ");
        Serial.println(ssid);
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED)
        {
            if (++local_count == 20)
            {
                ESP.restart();
            }
            Serial.print(".");
            delay(500);
        }
        wifi_reconnect_in_progress = false;
    }
}

void check_mqtt_connection()
{
    if (!mqtt_client.connected())
        setup_mqtt();
}

void setup()
{
    Serial.begin(115200);
    setup_wifi();
    setup_mqtt();
}

void loop()
{
    if (wifi_connected)
    {
        wifi_connected_loop();
    }
    else
    {
        delay(5000);
    }
}

void wifi_connected_loop()
{
    check_mqtt_connection();
    mqtt_client.loop();
}
