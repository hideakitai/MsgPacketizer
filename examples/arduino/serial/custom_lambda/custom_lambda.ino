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

    // handle received data with lambda which has incoming argument types/data
    MsgPacketizer::subscribe(Serial, RECV_INDEX,
        [&](const Nested& n) {
            // send same data back to app
            MsgPacketizer::send(Serial, SEND_INDEX, n);
        });
}

void loop() {
    // must be called to trigger callback and publish data
    // MsgPacketizer::update();

    // or you can call parse() and post() separately
    MsgPacketizer::parse();  // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
