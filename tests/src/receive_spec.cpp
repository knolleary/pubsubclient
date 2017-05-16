#include "PubSubClient.h"
#include "ShimClient.h"
#include "Buffer.h"
#include "Stream.h"
#include "BDDTest.h"
#include "trace.h"

#define MQTT_MAX_PACKET_SIZE 1024

IPAddress server(172, 16, 0, 2);

bool callback_called = false;
String lastTopic;
char lastPayload[1024];
unsigned int lastLength;

void reset_callback() {
    callback_called = false;
    lastTopic[0] = '\0';
    lastPayload[0] = '\0';
    lastLength = 0;
}

void callback(const MQTT::Publish& pub) {
    callback_called = true;
    lastTopic = pub.topic();
    memcpy(lastPayload, pub.payload(), pub.payload_len());
    lastLength = pub.payload_len();
}

uint8_t remaining_length_length(uint32_t remaining_length) {
    if (remaining_length < 128)
      return 1;
    else if (remaining_length < 16384)
      return 2;
    else if (remaining_length < 2097152)
      return 3;
    else
      return 4;
}

void add_remaining_length(uint8_t* packet, uint32_t remaining_length) {
    uint32_t pos = 1;
    do {
          uint8_t digit = remaining_length & 0x7f;
	  remaining_length >>= 7;
	  if (remaining_length)
	      digit |= 0x80;
	  packet[pos++] = digit;
    } while (remaining_length);
}

int test_receive_callback() {
    IT("receives a callback message");
    reset_callback();
    
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    
    byte publish[] = {0x30,0xe,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    shimClient.respond(publish,16);
    
    rc = client.loop();

    IS_TRUE(rc);
    
    IS_TRUE(callback_called);
    IS_TRUE(lastTopic == "topic");
    IS_TRUE(memcmp(lastPayload,"payload",7)==0);
    IS_TRUE(lastLength == 7);
    
    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_stream() {
    IT("receives a streamed callback message");
    reset_callback();
    
    Stream stream;
    stream.expect((uint8_t*)"payload",7);
    
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient, server, 1883); // stream?
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    
    byte publish[] = {0x30,0xe,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    shimClient.respond(publish,16);
    
    rc = client.loop();

    IS_TRUE(rc);
    
    IS_TRUE(callback_called);
    IS_TRUE(lastTopic == "topic");
    IS_TRUE(lastLength == 7);
    
    IS_FALSE(stream.error());
    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_max_sized_message() {
    IT("receives an max-sized message");
    reset_callback();
    
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    
    uint32_t length = MQTT_MAX_PACKET_SIZE;
    byte length_length = remaining_length_length(length - 2);
    byte payload[] = {0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    byte publish[1 + length_length + sizeof(payload)];
    publish[0] = 0x30;
    add_remaining_length(publish, length - 2);
    memcpy(publish + 1 + length_length, payload, sizeof(payload));

    byte bigPublish[length];
    memset(bigPublish,'A',length - 1);
    bigPublish[length - 1] = 'B';
    memcpy(bigPublish,publish,16);
    shimClient.respond(bigPublish,length);
    
    rc = client.loop();
    
    IS_TRUE(rc);
    
    IS_TRUE(callback_called);
    IS_TRUE(lastTopic == "topic");
    IS_TRUE(lastLength == length-9);
    IS_TRUE(memcmp(lastPayload, payload, lastLength)==0);
    
    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_oversized_message() {
    IT("drops an oversized message");
    reset_callback();
    
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    
    uint32_t length = MQTT_MAX_PACKET_SIZE+1;
    uint8_t length_length = remaining_length_length(length - 2);
    byte payload[] = {0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    byte publish[1 + length_length + sizeof(payload)];
    publish[0] = 0x30;
    add_remaining_length(publish, length - 2);
    memcpy(publish + 1 + length_length, payload, sizeof(payload));

    byte bigPublish[length];
    memset(bigPublish,'A',length - 1);
    bigPublish[length - 1] = 'B';
    memcpy(bigPublish,publish,16);
    shimClient.respond(bigPublish,length);
    
    rc = client.loop();
    
    IS_TRUE(rc);
    
    IS_FALSE(callback_called);
    
    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_oversized_stream_message() {
    IT("drops an oversized message");
    reset_callback();
    
    Stream stream;

    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient, server, 1883); // stream?
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    
    uint32_t length = MQTT_MAX_PACKET_SIZE+1;
    uint8_t length_length = remaining_length_length(length - 2);
    byte payload[] = {0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    byte publish[1 + length_length + sizeof(payload)];
    publish[0] = 0x30;
    add_remaining_length(publish, length - 2);
    memcpy(publish + 1 + length_length, payload, sizeof(payload));

    byte bigPublish[length];
    memset(bigPublish,'A',length - 1);
    bigPublish[length - 1] = 'B';
    memcpy(bigPublish,publish,16);
    
    shimClient.respond(bigPublish, length);
    stream.expect(payload, sizeof(payload));
    
    rc = client.loop();
    
    IS_TRUE(rc);
    
    IS_TRUE(callback_called);
    IS_TRUE(lastTopic == "topic");
    IS_TRUE(lastLength == length-9);
    
    IS_FALSE(stream.error());
    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_qos1() {
    IT("receives a qos1 message");
    reset_callback();
    
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    
    byte publish[] = {0x32,0x10,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x12,0x34,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    shimClient.respond(publish,18);
    
    byte puback[] = {0x40,0x2,0x12,0x34};
    shimClient.expect(puback,4);
    
    rc = client.loop();
    
    IS_TRUE(rc);
    
    IS_TRUE(callback_called);
    IS_TRUE(lastTopic == "topic");
    IS_TRUE(memcmp(lastPayload,"payload",7)==0);
    IS_TRUE(lastLength == 7);
    
    IS_FALSE(shimClient.error());

    END_IT
}

int main()
{
    test_receive_callback();
    test_receive_stream();
    test_receive_max_sized_message();
    test_receive_oversized_message();
    test_receive_oversized_stream_message();
    test_receive_qos1();
    
    FINISH
}
