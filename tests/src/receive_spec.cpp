#include "PubSubClient.h"
#include "ShimClient.h"
#include "Buffer.h"
#include "BDDTest.h"
#include "trace.h"


byte server[] = { 172, 16, 0, 2 };

bool callback_called = false;
char lastTopic[1024];
char lastPayload[1024];
unsigned int lastLength;
int testParams[2] = { 0, 0 };

struct TestArg {
    TestArg(int param1, int param2):
        param1_(param1),
        param2_(param2)
    {}

    int param1_;
    int param2_;
};

class AbstractTopicHandler {
    public:
        virtual ~AbstractTopicHandler() {}

        virtual bool operator()(byte* payload, unsigned int length) = 0;

};

struct TopicHandler: public AbstractTopicHandler {
        TopicHandler():
            AbstractTopicHandler()
        {}
        virtual ~TopicHandler() {}
        virtual bool operator()(byte* payload, unsigned int length) {
            memcpy(lastPayload,payload,length);
            lastLength = length;
            return true;
        }
};

struct AnotherTopicHandler: public AbstractTopicHandler {
        AnotherTopicHandler(int param):
            AbstractTopicHandler(),
            param_(param)
        {}

        virtual ~AnotherTopicHandler() {}
        
        virtual bool operator()(byte* payload, unsigned int length) {
            //extra params can be passed via ctr - param_ is an example
            memcpy(lastPayload,payload,length);
            lastLength = length;
            return true;
        }

        int param_;
};

class SubscriptionHandler {
    public:
        SubscriptionHandler(const char* topic, AbstractTopicHandler* handler):
            topic_(topic),
            handler_(handler)
        {}

        SubscriptionHandler():
            topic_(NULL),
            handler_(NULL)
        {}

        bool operator==(char* T2)
        {
            return strcmp(topic_, T2) == 0;
        }

        bool operator()(byte* payload, unsigned int length)
        {
            return (*handler_)(payload, length);
        }

        const char* get_topic() const
        {
            return topic_;
        }

    private:
        const char* topic_;
        AbstractTopicHandler* handler_;
};


struct SubscriptionHandlerTrampoline {
    //this can be dynamically allocated
    SubscriptionHandler sub_[10];
    unsigned int size_;
};



void reset_callback() {
    callback_called = false;
    lastTopic[0] = '\0';
    lastPayload[0] = '\0';
    lastLength = 0;
    testParams[0] = 0;
    testParams[1] = 0;
}

void callback(char* topic, byte* payload, unsigned int length) {
    callback_called = true;
    strcpy(lastTopic,topic);
    memcpy(lastPayload,payload,length);
    lastLength = length;
}

void callbackWithArg2(char* topic, byte* payload, unsigned int length, void* arg) {
    callback_called = true;
    SubscriptionHandlerTrampoline* tramp = static_cast<SubscriptionHandlerTrampoline*>(arg);
    //brute force search
    for(unsigned int counter = 0; counter < tramp->size_; ++counter) {
        if (tramp->sub_[counter] == topic) {
            strcpy(lastTopic,topic);
            tramp->sub_[counter](payload, length);
        }
    }
}

void callbackWithArg(char* topic, byte* payload, unsigned int length, void* arg) {
    callback_called = true;
    strcpy(lastTopic,topic);
    memcpy(lastPayload,payload,length);
    lastLength = length;
    testParams[0] = static_cast<TestArg*>(arg)->param1_;
    testParams[1] = static_cast<TestArg*>(arg)->param2_;
}

int test_receive_callback() {
    IT("receives a callback message");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);

    PubSubClient client(server, 1883, callback, shimClient);
    int rc = client.connect((char*)"client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30,0xe,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    shimClient.respond(publish,16);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic,"topic")==0);
    IS_TRUE(memcmp(lastPayload,"payload",7)==0);
    IS_TRUE(lastLength == 7);

    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_callback_with_arg() {
    IT("receives a callback with arg message");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);

    TestArg testInstance(1234321, 3333);

    PubSubClient client(server, 1883, callbackWithArg, static_cast<void*>(&testInstance), shimClient);
    int rc = client.connect((char*)"client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30,0xe,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    shimClient.respond(publish,16);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic,"topic")==0);
    IS_TRUE(memcmp(lastPayload,"payload",7)==0);
    IS_TRUE(lastLength == 7);
    IS_TRUE(testParams[0] == 1234321);
    IS_TRUE(testParams[1] == 3333);

    IS_FALSE(shimClient.error());

    END_IT
}

int test_receive_callback_with_arg_object_oriented() {
    IT("receives a callback with object oriented arg message");
    reset_callback();

    ShimClient shimClient;
    shimClient.setAllowConnect(true);

    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);

    TopicHandler topicA;
    AnotherTopicHandler topicB(321);
    SubscriptionHandlerTrampoline trampoline = { { SubscriptionHandler("topic", &topicA), SubscriptionHandler("topicB", &topicB) }, 2 };

    PubSubClient client(server, 1883, callbackWithArg2, static_cast<void*>(&trampoline), shimClient);
    int rc = client.connect((char*)"client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30,0xe,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    shimClient.respond(publish,16);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic,"topic")==0);
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

    PubSubClient client(server, 1883, callback, shimClient, stream);
    int rc = client.connect((char*)"client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x30,0xe,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    shimClient.respond(publish,16);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic,"topic")==0);
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

    PubSubClient client(server, 1883, callback, shimClient);
    int rc = client.connect((char*)"client_test1");
    IS_TRUE(rc);

    int length = MQTT_MAX_PACKET_SIZE;
    byte publish[] = {0x30,length-2,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    byte bigPublish[length];
    memset(bigPublish,'A',length);
    bigPublish[length] = 'B';
    memcpy(bigPublish,publish,16);
    shimClient.respond(bigPublish,length);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic,"topic")==0);
    IS_TRUE(lastLength == length-9);
    IS_TRUE(memcmp(lastPayload,bigPublish+9,lastLength)==0);

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

    PubSubClient client(server, 1883, callback, shimClient);
    int rc = client.connect((char*)"client_test1");
    IS_TRUE(rc);

    int length = MQTT_MAX_PACKET_SIZE+1;
    byte publish[] = {0x30,length-2,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    byte bigPublish[length];
    memset(bigPublish,'A',length);
    bigPublish[length] = 'B';
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

    PubSubClient client(server, 1883, callback, shimClient, stream);
    int rc = client.connect((char*)"client_test1");
    IS_TRUE(rc);

    int length = MQTT_MAX_PACKET_SIZE+1;
    byte publish[] = {0x30,length-2,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};

    byte bigPublish[length];
    memset(bigPublish,'A',length);
    bigPublish[length] = 'B';
    memcpy(bigPublish,publish,16);

    shimClient.respond(bigPublish,length);
    stream.expect(bigPublish+9,length-9);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic,"topic")==0);
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

    PubSubClient client(server, 1883, callback, shimClient);
    int rc = client.connect((char*)"client_test1");
    IS_TRUE(rc);

    byte publish[] = {0x32,0x10,0x0,0x5,0x74,0x6f,0x70,0x69,0x63,0x12,0x34,0x70,0x61,0x79,0x6c,0x6f,0x61,0x64};
    shimClient.respond(publish,18);

    byte puback[] = {0x40,0x2,0x12,0x34};
    shimClient.expect(puback,4);

    rc = client.loop();

    IS_TRUE(rc);

    IS_TRUE(callback_called);
    IS_TRUE(strcmp(lastTopic,"topic")==0);
    IS_TRUE(memcmp(lastPayload,"payload",7)==0);
    IS_TRUE(lastLength == 7);

    IS_FALSE(shimClient.error());

    END_IT
}

int main()
{
    SUITE("Receive");
    test_receive_callback();
    test_receive_callback_with_arg();
    test_receive_callback_with_arg_object_oriented();
    test_receive_stream();
    test_receive_max_sized_message();
    test_receive_oversized_message();
    test_receive_oversized_stream_message();
    test_receive_qos1();

    FINISH
}
