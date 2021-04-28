#include <ArduinoJson.h>  // include before MsgPacketizer.h

#define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>
#include <WiFi.h>

const uint8_t msg_index = 0x12;

// WiFi stuff
const char* ssid = "your-ssid";
const char* pwd = "your-password";
const IPAddress ip(192, 168, 0, 201);
const IPAddress gateway(192, 168, 0, 1);
const IPAddress subnet(255, 255, 255, 0);

const char* host = "192.168.0.201";  // loop back
const int port = 54321;

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

    // only subscribing ArduinoJson with lambda is allowed
    // because subscribing JsonDocument as global object is not reccomended
    // see https://arduinojson.org/v6/how-to/reuse-a-json-document/
    MsgPacketizer::subscribe(client, msg_index,
        [&](const StaticJsonDocument<200>& doc) {
            Serial.print(doc["us"].as<uint32_t>());
            Serial.print(" ");
            Serial.print(doc["usstr"].as<String>());
            Serial.print(" [");
            Serial.print(doc["now"][0].as<double>());
            Serial.print(" ");
            Serial.print(doc["now"][1].as<double>());
            Serial.println("]");
        });

    // you can also use DynamicJsonDocument if you want
    // MsgPacketizer::subscribe(client, msg_index,
    //     [&](const DynamicJsonDocument& doc) {
    //         Serial.print(doc["us"].as<uint32_t>());
    //         Serial.print(" ");
    //         Serial.print(doc["usstr"].as<String>());
    //         Serial.print(" [");
    //         Serial.print(doc["now"][0].as<double>());
    //         Serial.print(" ");
    //         Serial.print(doc["now"][1].as<double>());
    //         Serial.println("]");
    //     });

    // or of course you can bind to variables directly
    // MsgPacketizer::subscribe(client, msg_index,
    //     [&](const MsgPack::map_size_t,
    //         const String& k_us, const uint32_t v_us,
    //         const String& k_usstr, const String& v_usstr,
    //         const String& k_now, const MsgPack::arr_size_t, const double now0, const double now1) {
    //         Serial.print(v_us);
    //         Serial.print(" ");
    //         Serial.print(v_usstr);
    //         Serial.print(" [");
    //         Serial.print(now0);
    //         Serial.print(" ");
    //         Serial.print(now1);
    //         Serial.println("]");
    //     });
}

void loop() {
    static uint32_t prev_ms = millis();
    if (millis() > prev_ms + 1000) {
        prev_ms = millis();

        uint32_t now = micros();

        StaticJsonDocument<200> doc;
        // you can also use DynamicJsonDocument if you want
        // DynamicJsonDocument doc(200);
        doc["us"] = now;
        doc["usstr"] = String(now) + "[us]";
        JsonArray data = doc.createNestedArray("now");
        data.add(now * 0.001);
        data.add(now * 0.001 * 0.001);

        // only `send` ArduinoJson is supported (you can `publish` but not recommended)
        // because ArduinoJson is designed to be throw-away objects
        // please use standard `publish` without ArduinoJson
        // see https://arduinojson.org/v6/how-to/reuse-a-json-document/ for the detail
        MsgPacketizer::send(client, host, port, msg_index, doc);

        // or you can send them directly
        // MsgPacketizer::send(client, host, port, msg_index,
        //     MsgPack::map_size_t(3),
        //     "us", now,
        //     "usstr", String(now) + "[us]",
        //     "now", MsgPack::arr_size_t(2), now * 0.001, now * 0.001 * 0.001);
    }

    // must be called to trigger callback and publish data
    MsgPacketizer::update();
}
