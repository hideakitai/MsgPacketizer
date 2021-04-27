// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>
#include <Ethernet.h>

const uint8_t msg_index = 0x12;

// Ethernet stuff
uint8_t mac[] = {0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45};
const IPAddress ip(192, 168, 0, 201);
// Ethernet with useful options
// const IPAddress dns (192, 168, 0, 1);
// const IPAddress gateway (192, 168, 0, 1);
// const IPAddress subnet (255, 255, 255, 0);

const char* host = "192.168.0.17";  // loop back
const int port = 54321;

int i;
float f;
String s;

EthernetUDP client;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // Ethernet stuff
    Ethernet.begin(mac, ip);
    // Ethernet with useful options
    // Ethernet.begin(mac, ip, dns, gateway, subnet); // full
    // Ethernet.setRetransmissionCount(4); // default: 8[times]
    // Ethernet.setRetransmissionTimeout(50); // default: 200[ms]

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
