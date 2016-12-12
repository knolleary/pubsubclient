/*
 *  Handling of subscriptions in an object oriented manner
 *
 *  This example shows the usage of a parametrized callback. It's up to the
 *  developer to decide about design of topic handlers. Presented implementation
 *  explores one of many possibilities how the design may look like. 
 *
 *  Steps to remember:
 *      1. Setup your transport layer (RF24 in this case)
 *      2. Instantiate handlers for desired topic and aggregate them into a
 *         container object
 *      3. Create PubSub client instance - pass pointer to a callback function
 *         and pointer to the container
 *      4. Callback should implement a possibility to dispatch payload to a
 *         desired handler instance when the topic of interest is changed
 *
 */
#include <PubSubClient.h>
#include <RF24.h>
#include <RF24Ethernet.h>
#include <RF24Mesh.h>
#include <RF24Network.h>

const unsigned int MAX_PAYLOAD_BUFFER_SIZE = 128;

// RF24 setup
RF24 radio{ 7, 8 };
RF24Network network(radio);
RF24Mesh mesh(radio, network);
RF24EthernetClass RF24Ethernet(radio, network, mesh);

namespace RF24Net
{
void start(const IPAddress& my_ip, const IPAddress& gateway_ip, RF24Mesh& mesh_ref)
{
    Ethernet.begin(my_ip);
    Ethernet.set_gateway(gateway_ip);

    if (mesh_ref.begin()) {
        Serial.println(F("Mesh OK"));
    } else {
        Serial.println(F("Mesh failed"));
    }
}

void checkState(RF24Mesh& mesh_ref)
{
    static uint32_t mesh_timer = 0;
    if (millis() - mesh_timer > 30000) { //Every 30 seconds, test mesh connectivity
        mesh_timer = millis();
        if (!mesh_ref.checkConnection()) {
            mesh_ref.renewAddress();
        }
    }
}
}

// Topic handlers implementation
class AbstractTopicHandler;

struct SubscriptionHandlerTrampoline {
    // This can be allocated in better way
    AbstractTopicHandler* sub_[10];
    unsigned int size_;
};

class AbstractTopicHandler {
    public:
        AbstractTopicHandler(const char* topic):
            subscribedTopic_(topic)
        {}

        virtual ~AbstractTopicHandler()
        {}

        bool operator==(char* T2)
        {
            return strcmp(subscribedTopic_, T2) == 0;
        }

        virtual void operator()(byte* payload, unsigned int length) = 0;

        //Callback function that needs to be passed when PubSub client is created.
        //It doesn't have to be a static class member. It can be also a public free function
        static void callback(char* topic, byte* payload, unsigned int length, void* arg) {
            SubscriptionHandlerTrampoline* tramp = static_cast<SubscriptionHandlerTrampoline*>(arg);
            //brute force search
            for(unsigned int counter = 0; counter < tramp->size_; ++counter) {
                if (*(tramp->sub_[counter]) == topic) {
                    tramp->sub_[counter]->operator()(payload, length);
                }
            }
        }

        const char* getTopic() const{
            return subscribedTopic_;
        }

    protected:
        const char* subscribedTopic_;

};

struct TopicHandler: public AbstractTopicHandler {
        TopicHandler():
            AbstractTopicHandler("topic/param1")
        {}

        virtual ~TopicHandler() {}

        virtual void operator()(byte* payload, unsigned int length) {
            if(length < MAX_PAYLOAD_BUFFER_SIZE) {
                memset(buffer,0,sizeof(buffer));
                memcpy(buffer, payload, length);
            }
        }

        const char* getString() const {
            return buffer;
        }

        char buffer[MAX_PAYLOAD_BUFFER_SIZE];
};

struct AnotherTopicHandler: public AbstractTopicHandler {
        AnotherTopicHandler(long multiplier):
            AbstractTopicHandler("topic/param2"),
            multiplier_(multiplier),
            param_(0)
        {}

        virtual ~AnotherTopicHandler() {}
        
        virtual void operator()(byte* payload, unsigned int length) {
            if(length < MAX_PAYLOAD_BUFFER_SIZE) {
                memset(buffer,0,sizeof(buffer));
                memcpy(buffer, payload, length);
                param_ = strtol(buffer, NULL, 10);
            }
        }

        long getValue() const {
            return param_ * multiplier_;
        }

        char buffer[MAX_PAYLOAD_BUFFER_SIZE];
        long multiplier_;
        long param_;
};
// End of topic handler implementation

IPAddress ip(10, 10, 3, 30);
IPAddress gateway(10, 10, 3, 13);
IPAddress server(10, 10, 3, 13);

EthernetClient ethClient;

// Instantiation of handlers
TopicHandler topicA;
AnotherTopicHandler topicB(2);
SubscriptionHandlerTrampoline trampoline = { { &topicB, &topicA }, 2 };

PubSubClient client(server, 1883, AbstractTopicHandler::callback, static_cast<void*>(&trampoline), ethClient);

void setup()
{
    Serial.begin(115200);
    // Network setup
    RF24Net::start(ip, gateway, mesh);
    RF24Net::checkState(mesh);
    
    // MQTT connection and subscription
    while(!client.connect("arduinoClient")) { delay(100); };
    Serial.println("connected");
    while(!client.subscribe(topicA.getTopic())) { delay(100); };
    while(!client.subscribe(topicB.getTopic())) { delay(100); };
    Serial.println("subscribed");

}

void loop()
{
    RF24Net::checkState(mesh);

    Serial.print(String(topicA.getTopic()) + " value is ");
    Serial.println(topicA.getString());

    Serial.print(String(topicB.getTopic()) + " value is ");
    Serial.println(topicB.getValue());

    delay(1000);
    client.loop();
}
