#include "PubSubClient.h"
#include "ShimClient.h"
#include "Buffer.h"
#include "BDDTest.h"
#include "trace.h"


IPAddress server(172, 16, 0, 2);

void callback(const MQTT::Publish& pub) {
  // handle message arrived
}


int test_connect_fails_no_network() {
    IT("fails to connect if underlying client doesn't connect");
    ShimClient shimClient;
    shimClient.setAllowConnect(false);
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_FALSE(rc);
    END_IT
}

int test_connect_fails_on_no_response() {
    IT("fails to connect if no response received after 15 seconds");
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_FALSE(rc);
    END_IT
}

int test_connect_properly_formatted() {
    IT("sends a properly formatted connect packet and succeeds");
    ShimClient shimClient;
    
    shimClient.setAllowConnect(true);
    IPAddress expectServer(172, 16, 0, 2);
    shimClient.expectConnect(expectServer,1883);
    byte connect[] = { 0x10,		// type and flags
		       0x18,		// length
		       0x00,0x04,0x4d,0x51,0x54,0x54,	// protocol name
		       0x04,		// protocol level
		       0x02,		// flags
		       0x00,0x0f,	// keepalives
		       0x0,0xc,0x63,0x6c,0x69,0x65,0x6e,0x74,0x5f,0x74,0x65,0x73,0x74,0x31 // client id
    };
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };

    shimClient.expect(connect,28);
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());
    
    END_IT
}

int test_connect_properly_formatted_hostname() {
    IT("accepts a hostname");
    ShimClient shimClient;
    
    shimClient.setAllowConnect(true);
    shimClient.expectConnect((char* const)"localhost",1883);
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient, "localhost", 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());
    
    END_IT
}


int test_connect_fails_on_bad_rc() {
    IT("fails to connect if a bad return code is received");
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    byte connack[] = { 0x20, 0x02, 0x00, 0x01 };
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_FALSE(rc);
    END_IT
}

int test_connect_accepts_username_password() {
    IT("accepts a username and password");
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connect[] = { 0x10,		// type and flags
		       0x24,		// length
		       0x00,0x04,0x4d,0x51,0x54,0x54,	// protocol name
		       0x04,		// protocol level
		       0xc2,		// flags
		       0x0,0xf,		// keepalives
		       0x0,0xc,0x63,0x6c,0x69,0x65,0x6e,0x74,0x5f,0x74,0x65,0x73,0x74,0x31,	// client id
		       0x0,0x4,0x75,0x73,0x65,0x72,	// username
		       0x0,0x4,0x70,0x61,0x73,0x73	// password
    };
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.expect(connect,0x28);
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect(MQTT::Connect("client_test1")
			    .set_auth("user", "pass"));
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());

    END_IT
}

int test_connect_accepts_username_no_password() {
    IT("accepts a username but no password");
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connect[] = { 0x10,		// type and flags
		       0x1e,		// length
		       0x0,0x4,0x4d,0x51,0x54,0x54,	// protocol name
		       0x04,		// protocol level
		       0x82,		// flags
		       0x0,0xf,		// keepalives
		       0x0,0xc,0x63,0x6c,0x69,0x65,0x6e,0x74,0x5f,0x74,0x65,0x73,0x74,0x31,	// client id
		       0x0,0x4,0x75,0x73,0x65,0x72	// username
    };
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.expect(connect,0x22);
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect(MQTT::Connect("client_test1")
			    .set_auth("user", ""));
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());

    END_IT
}

int test_connect_ignores_password_no_username() {
    IT("ignores a password but no username");
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connect[] = { 0x10,		// type and flags
		       0x18,		// length
		       0x0,0x04,0x4d,0x51,0x54,0x54,	// protocol name
		       0x4,		// protocol level
		       0x2,		// flags
		       0x0,0xf,		// keepalives
		       0x0,0xc,0x63,0x6c,0x69,0x65,0x6e,0x74,0x5f,0x74,0x65,0x73,0x74,0x31	// client id
    };
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.expect(connect,28);
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect(MQTT::Connect("client_test1")
			    .set_auth("", "pass"));
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());

    END_IT
}

int test_connect_with_will() {
    IT("accepts a will");
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connect[] = { 0x10,		// type and flags
		       0x30,		// length
		       0x0,0x4,0x4d,0x51,0x54,0x54,	// protocol name
		       0x4,		// protocol level
		       0xe,		// flags
		       0x0,0xf,		// keepalives
		       0x0,0xc,0x63,0x6c,0x69,0x65,0x6e,0x74,0x5f,0x74,0x65,0x73,0x74,0x31,	// client id
		       0x0,0x9,0x77,0x69,0x6c,0x6c,0x54,0x6f,0x70,0x69,0x63,		// will topic
		       0x0,0xb,0x77,0x69,0x6c,0x6c,0x4d,0x65,0x73,0x73,0x61,0x67,0x65	// will message
    };
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.expect(connect,0x34);
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect(MQTT::Connect("client_test1")
			    .set_will("willTopic", "willMessage", 1, false));
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());

    END_IT
}

int test_connect_with_will_username_password() {
    IT("accepts a will, username and password");
    ShimClient shimClient;
    shimClient.setAllowConnect(true);
    
    byte connect[] = { 0x10,		// type and flags
		       0x40,		// length
		       0x0,0x4,0x4d,0x51,0x54,0x54,	// protocol name
		       0x4,		// protocol level
		       0xce,		// flags
		       0x0,0xf,		// keepalives
		       0x0,0xc,0x63,0x6c,0x69,0x65,0x6e,0x74,0x5f,0x74,0x65,0x73,0x74,0x31,	// client id
		       0x0,0x9,0x77,0x69,0x6c,0x6c,0x54,0x6f,0x70,0x69,0x63,		// will topic
		       0x0,0xb,0x77,0x69,0x6c,0x6c,0x4d,0x65,0x73,0x73,0x61,0x67,0x65,	// will message
		       0x0,0x4,0x75,0x73,0x65,0x72,	// username
		       0x0,0x8,0x70,0x61,0x73,0x73,0x77,0x6f,0x72,0x64	// password
    };
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };
    shimClient.expect(connect,0x44);
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect(MQTT::Connect("client_test1")
			    .set_auth("user", "password")
			    .set_will("willTopic", "willMessage", 1, false));
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());

    END_IT
}

int test_connect_disconnect_connect() {
    IT("connects, disconnects and connects again");
    ShimClient shimClient;
    
    shimClient.setAllowConnect(true);
    IPAddress expectServer(172, 16, 0, 2);
    shimClient.expectConnect(expectServer,1883);
    byte connect[] = { 0x10,		// type and flags
		       0x18,		// length
		       0x0,0x4,0x4d,0x51,0x54,0x54,	// protocol name
		       0x4,		// protocol level
		       0x2,		// flags
		       0x0,0xf,		// keepalives
		       0x0,0xc,0x63,0x6c,0x69,0x65,0x6e,0x74,0x5f,0x74,0x65,0x73,0x74,0x31	// client id
    };
    byte connack[] = { 0x20, 0x02, 0x00, 0x00 };

    shimClient.expect(connect,28);
    shimClient.respond(connack,4);
    
    PubSubClient client(shimClient,server, 1883);
    client.set_callback(callback);
    int rc = client.connect("client_test1");
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());
    
    byte disconnect[] = {0xE0,0x00};
    shimClient.expect(disconnect,2);

    client.disconnect();
    
    IS_FALSE(client.connected());
    IS_FALSE(shimClient.connected());
    IS_FALSE(shimClient.error());
    
    shimClient.expect(connect,28);
    shimClient.respond(connack,4);
    rc = client.connect("client_test1");
    IS_TRUE(rc);
    IS_FALSE(shimClient.error());
    
    END_IT
}

int main()
{
    test_connect_fails_no_network();
    test_connect_fails_on_no_response();
    test_connect_properly_formatted();
    test_connect_fails_on_bad_rc();
    test_connect_properly_formatted_hostname();
    test_connect_accepts_username_password();
    test_connect_accepts_username_no_password();
    test_connect_ignores_password_no_username();
    test_connect_with_will();
    test_connect_with_will_username_password();
    test_connect_disconnect_connect();
    
    FINISH
}
