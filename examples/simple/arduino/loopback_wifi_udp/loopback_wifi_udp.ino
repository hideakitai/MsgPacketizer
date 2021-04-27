// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>
#include <WiFi.h>

const uint8_t msg_index = 0x12;

// WiFi stuff
const char* ssid = "your-ssid";
const char* pwd = "your-password";
const IPAddress ip(192, 168, 0, 201);
const IPAddress gateway(192, 168, 0, 1);
const IPAddress subnet(255, 255, 255, 0);

const char* host = "192.168.0.17";  // loop back
const int port = 54321;

int i;
float f;
String s;

WiFiUDP client;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // WiFi stuff (no timeout setting for WiFi)
#ifdef ESP_PLATFORM
    WiFi.disconnect(true, true);  // disable wifi, erase ap info
    delay(1000);
    WiFi.mode(WIFI_STA);
#endif
    WiFi.begin(ssid, pwd);
    WiFi.config(ip, gateway, subnet);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.print("WiFi connected, IP = ");
    Serial.println(WiFi.localIP());

    // start client
    client.begin(port);

    MsgPacketizer::publish(client, host, port, msg_index, i, f, s)->setFrameRate(1);

    MsgPacketizer::subscribe(client, msg_index,
        [&](const int i, const float f, const String& s) {
            Serial.print(i);
            Serial.print(" ");
            Serial.print(f);
            Serial.print(" ");
            Serial.println(s);
        });
}

void loop() {
    uint32_t now = millis();
    i = now;
    f = now * 0.001;
    s = String(f) + "[sec]";

    // must be called to trigger callback and publish data
    MsgPacketizer::update();
}
