// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// comment out if you want to use TCP
// #define USE_TCP

const uint8_t RECV_INDEX = 0x01;
const uint8_t SEND_INDEX = 0x02;

// WiFi stuff
const char* SSID = "your-ssid";
const char* PASSWORD = "your-password";
const IPAddress IP_ADDR(192, 168, 0, 201);
const IPAddress GATEWAY(192, 168, 0, 1);
const IPAddress SUBNET(255, 255, 255, 0);

const char* HOST = "192.168.0.200";
#ifdef USE_TCP
const int TCP_PORT = 55555;
WiFiServer server;
WiFiClient client;
#else
const int RECV_PORT = 55555;
const int SEND_PORT = 54321;
WiFiUDP client;
#endif

void setup() {
    Serial.begin(115200);
    delay(2000);

    // WiFi stuff (no timeout setting for WiFi)
#ifdef ESP_PLATFORM
    WiFi.disconnect(true, true);  // disable wifi, erase ap info
    delay(1000);
    WiFi.mode(WIFI_STA);
#endif
    WiFi.begin(SSID, PASSWORD);
    WiFi.config(IP_ADDR, GATEWAY, SUBNET);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.print("WiFi connected, IP = ");
    Serial.println(WiFi.localIP());

    // start client
#ifdef USE_TCP
    server.begin(TCP_PORT);
    while (!client) {
        client = server.available();
        delay(500);
        Serial.print(".");
    }
    Serial.print("TCP connection success, PORT = ");
    Serial.println(TCP_PORT);
#else
    client.begin(RECV_PORT);
#endif

    // handle received data with lambda which has incoming argument types/data
    // if you want to communicate with data based on json array,
    // please do not forget to add MsgPack::arr_size_t
    MsgPacketizer::subscribe(client, RECV_INDEX,
        [&](const MsgPack::arr_size_t& sz, const int i, const float f, const String& s) {
            if (sz.size() == 3) {  // if array size is correct
                // send same data back to app
#ifdef USE_TCP
                MsgPacketizer::send_arr(client, SEND_INDEX, i, f, s);
#else
                MsgPacketizer::send_arr(client, HOST, SEND_PORT, SEND_INDEX, i, f, s);
#endif
            }
        });
}

void loop() {
    // must be called to trigger callback and publish data
    // MsgPacketizer::update();

    // or you can call parse() and post() separately
    MsgPacketizer::parse();  // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
