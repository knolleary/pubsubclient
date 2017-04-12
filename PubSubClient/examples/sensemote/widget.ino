/*
This demo uses MQTT to provide a real-time web frontend to the Arduino.
The HTML page below is pushed to test.mosquitto.org.
A presence topic and some communication endpoints are created under sensemote/widgets
MQTT is accessed on the web using the HTTP bridge at http://sensemote.com:8081

Change WIDGET_NAME below, upload this sketch to the Arduino (+ ethernet shield) then browse to http://www.sensemote.com:8081/presence.html.

You must use the version of the PubSubClient library provided.

The Arduino outputs received messages on the serial port and sends an increasing counter at 1Hz.
*/

#include <PubSubClient.h>

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <avr/pgmspace.h>

/**** CHANGE THIS *************/
// Then browse to http://www.sensemote.com:8081/presence.html
#define WIDGET_NAME "TestWidget"
// No spaces allowed
/******************************/

byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x04 };
IPAddress ip;
PubSubClient client;
unsigned long lastTime = 0;
int counter = 0;

// This is the HTML which provides the frontend on the web
char PROGMEM html_progmem[] =
"<html>"
"<head>"
"<style>"
"#opaque"
"{"
"    position: fixed;"
"    top: 0px;"
"    left: 0px;"
"    width: 100%;"
"    height: 100%;"
"    z-index: 99;"
"    display: block;"
"    background-color: black;"
"    filter: alpha(opacity=40);"
"    opacity: 0.4;"
"}"
"* html #opaque"
"{"
"    position: absolute;"
"}"
"</style>"
"</head>"
"<h1>Widget Demo</h1>"
"<div id='opaque'></div>"
"<form>"
"    <label>Out</label><input type='textarea' value='' id='messageOut'/>"
"    <button class='btn' type='button' id='publish'>Send</button>"
"    <br>"
"    <label>In</label><input type='textarea' value='' id='messageIn' disabled/>"
"</form>"
""
"<script type='text/javascript' src='/jquery.min.js'></script>"
"<script type='text/javascript' src='/api2.js'></script>"
"<script type='text/javascript'>"
""
"var WIDGET_NAME = '" WIDGET_NAME "';"
"var widgetOutTopic = 'sensemote/widgets/'+WIDGET_NAME+'/out';"
"var widgetInTopic = 'sensemote/widgets/'+WIDGET_NAME+'/in';"
"var widgetPresenceTopic = 'sensemote/widgets/'+WIDGET_NAME+'/presence';"
""
"var timer;"
""
"function reconnect()"
"{"
"    clearTimeout(timer);"
"    timer = setTimeout(function(){startStreaming();}, 1000);"
"}"
""
"function startStreaming()"
"{"
"    $('#opaque').css({display:'block'});"
""
"    sensemote.register({"
"        data: function(topic, msg) {"
"            if (topic == widgetPresenceTopic) {"
"                if (msg == '1') {"
"                    $('#opaque').css({display:'none'});"
"                }"
"                else {"
"                    $('#opaque').css({display:'block'});"
"                }"
"            }"
"            else"
"            if (topic == widgetInTopic) {"
"                $('#messageIn').val(msg);"
"            }"
"        },"
"        error: function() {reconnect();},"
"    });"
"    sensemote.subscribe(widgetPresenceTopic, {"
"        success: function() {"
"            sensemote.subscribe(widgetInTopic, {success: sensemote.stream});"
"        }"
"    });"
"}"
""
"$(window).load(function()"
"{"
"    $('#publish').click(function() {"
"        sensemote.publish(widgetOutTopic, $('#messageOut').val());"
"    });"
"    startStreaming();"
"});"
"</script>"
"</body>"
"</html>";

void callback(char* topic, uint8_t* payload, unsigned int length)
{
    Serial.print("RX: ");
    Serial.print(topic);
    Serial.print(" -> ");
    while(length--)
        Serial.print((char)*payload++);
    Serial.println("");
}

void setup()
{
    Serial.begin(9600);

    Serial.println("Trying to get an IP address using DHCP");
    if (Ethernet.begin(mac) == 0)
    {
        Serial.println("Failed to configure Ethernet using DHCP");
        return;
    }

    Serial.print("My IP address: ");
    ip = Ethernet.localIP();
    for (byte thisByte = 0; thisByte < 4; thisByte++)
    {
        Serial.print(ip[thisByte], DEC);
        Serial.print(".");
    }
    Serial.println();

    client = PubSubClient("test.mosquitto.org", 1883, callback);
    mqttConnect();
}

void loop()
{
    unsigned long now = millis();
    char s[20];

    if (now - lastTime > 1000)
    {
        itoa(counter++, s, 10);
        client.publish("sensemote/widgets/" WIDGET_NAME "/in", s);
        lastTime = millis();
    }

    mqttConnect();
}

void mqttConnect()
{
    if (!client.connected())
    {
        client.connect(WIDGET_NAME, "sensemote/widgets/" WIDGET_NAME "/presence", 1, 1, "");
        client.publish("sensemote/widgets/" WIDGET_NAME "/presence", (uint8_t *)"1", 2, true);
        client.publish_P("sensemote/widgets/" WIDGET_NAME "/data", (uint8_t PROGMEM *)html_progmem, strlen_P(html_progmem), true);
        client.subscribe("sensemote/widgets/" WIDGET_NAME "/out");
    }
    client.loop();
}

