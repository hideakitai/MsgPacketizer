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
MsgPack::arr_t<int> arr {0, 0, 0}; // json: [0, 0, 0]

const uint8_t RECV_INDEX = 0x11;
const uint8_t SEND_INDEX = 0x12;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // update received data directly
    MsgPacketizer::subscribe(Serial, RECV_INDEX, arr);

    // register variables to publish repeatedly
    MsgPacketizer::publish(Serial, SEND_INDEX, arr)
        ->setFrameRate(10.f);
}

void loop() {
    // must be called to trigger callback and publish data
    MsgPacketizer::update();

    // or you can call parse() and post() separately
    // MsgPacketizer::parse();  // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
