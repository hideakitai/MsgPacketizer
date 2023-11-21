#include <ArduinoJson.h>  // include before MsgPacketizer.h

// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>

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

void setup() {
    Serial.begin(115200);
    delay(2000);

    // only subscribing ArduinoJson with lambda is allowed
    // because subscribing JsonDocument as global object is not reccomended
    // see https://arduinojson.org/v6/how-to/reuse-a-json-document/
    MsgPacketizer::subscribe(Serial, RECV_INDEX,
        // you can use DynamicJsonDocument<N> as well
        [&](const StaticJsonDocument<200>& doc) {
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
            // please use `publish` without ArduinoJson
            // see https://arduinojson.org/v6/how-to/reuse-a-json-document/ for the detail
            MsgPacketizer::send(Serial, SEND_INDEX, nested);
            // you can send Static/DynamicJsonDocument<N> as well
            // MsgPacketizer::send(Serial, SEND_INDEX, doc);
        });
}

void loop() {
    // must be called to trigger callback and publish data
    // MsgPacketizer::update();

    // or you can call parse() and post() separately
    MsgPacketizer::parse();  // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
