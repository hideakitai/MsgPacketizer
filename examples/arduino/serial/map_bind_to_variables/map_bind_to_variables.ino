// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>

// for STL enabled boards:
// MsgPack::arr_t<T> = std::vector<T>
// MsgPack::map_t<T, U> = std::map<T, U>
// MsgPack::bin_t<T> = std::vector<T = uint8_t || char>

// for NO-STL boards(AVR, megaAVR, SAMD, SPRESENSE):
// MsgPack::arr_t<T> = arx::stdx::vector<T>
// MsgPack::map_t<T, U> = arx::stdx::map<T, U>
// MsgPack::bin_t<T> = arx::stdx::vector<T = uint8_t || char>

// msgpac input && output
MsgPack::map_t<String, int> mp {{"a", 1}, {"b", 2}, {"c", 3}};  // json {{"a", 1}, {"b", 2}, {"c", 3}}

const uint8_t RECV_INDEX = 0x21;
const uint8_t SEND_INDEX = 0x22;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // update received data directly
    MsgPacketizer::subscribe(Serial, RECV_INDEX, mp);

    // register variables to publish repeatedly
    MsgPacketizer::publish(Serial, SEND_INDEX, mp)
        ->setFrameRate(10.f);
}

void loop() {
    // must be called to trigger callback and publish data
    MsgPacketizer::update();

    // or you can call parse() and post() separately
    // MsgPacketizer::parse();  // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
