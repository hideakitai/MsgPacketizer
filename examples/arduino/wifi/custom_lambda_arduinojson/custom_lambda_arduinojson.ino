#include <ArduinoJson.h>  // include before MsgPacketizer.h

#define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>
#include <WiFi.h>

// comment out if you want to use TCP
// #define USE_TCP

// json [i, f, s]
struct Array {
    int i;
    float f;
    String s;
    MSGPACK_DEFINE(i, f, s);
};

// {
//   "ka": [int, float, str], // micros, millis, seconds[sec]
//   "km": {"micros": int, "millis": int, "seconds": int}
// }
struct Nested {
    String ka;
    Array va;
    String km;
    MsgPack::map_t<String, int> vm;
    MSGPACK_DEFINE_MAP(ka, va, km, vm);
};

const uint8_t RECV_INDEX = 0x31;
const uint8_t SEND_INDEX = 0x32;

Nested nested;

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

    // only subscribing ArduinoJson with lambda is allowed
    // because subscribing JsonDocument as global object is not reccomended
    // see https://arduinojson.org/v6/how-to/reuse-a-json-document/
    MsgPacketizer::subscribe(client, RECV_INDEX,
        // you can use DynamicJsonDocument<N> as well
        [&](const StaticJsonDocument<200>& doc) {
            // dump json
            // serializeJson(doc, Serial);

            if (doc.containsKey("ka")) {
                nested.ka = "ka";
                nested.va.i = doc["ka"][0].as<int>();
                nested.va.f = doc["ka"][1].as<float>();
                nested.va.s = doc["ka"][2].as<String>();
            }
            if (doc.containsKey("km")) {
                nested.km = "km";
                nested.vm["micros"] = doc["km"]["micros"].as<int>();
                nested.vm["millis"] = doc["km"]["millis"].as<int>();
                nested.vm["seconds"] = doc["km"]["seconds"].as<int>();
            }

            // send same data back to app
            // only `send` ArduinoJson is supported (you can `publish` but not recommended)
            // because ArduinoJson is designed to be throw-away objects
            // please use standard `publish` without ArduinoJson
            // see https://arduinojson.org/v6/how-to/reuse-a-json-document/ for the detail
            // you can send Static/DynamicJsonDocument<N> as well
#ifdef USE_TCP
            MsgPacketizer::send(client, SEND_INDEX, nested);
            // MsgPacketizer::send(client, SEND_INDEX, doc);
#else
            MsgPacketizer::send(client, HOST, SEND_PORT, SEND_INDEX, nested);
            // MsgPacketizer::send(client, HOST, SEND_PORT, SEND_INDEX, doc);
#endif
        });
}

void loop() {
    // must be called to trigger callback and publish data
    // MsgPacketizer::update();

    // or you can call parse() and post() separately
    MsgPacketizer::parse();  // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
