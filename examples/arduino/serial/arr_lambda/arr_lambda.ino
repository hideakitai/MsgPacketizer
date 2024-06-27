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

const uint8_t RECV_INDEX = 0x11;
const uint8_t SEND_INDEX = 0x12;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // handle received data with lambda which has incoming argument types/data
    // if you want to communicate with data based on json array,
    // please do not forget to add MsgPack::arr_size_t
    MsgPacketizer::subscribe(Serial, RECV_INDEX,
        [&](const MsgPack::arr_size_t& sz, const int i1, const int i2, const int i3) {
            if (sz.size() == 3) {  // if array size is correct
                // send same data back to app
                MsgPacketizer::send_arr(Serial, SEND_INDEX, i1, i2, i3);
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
