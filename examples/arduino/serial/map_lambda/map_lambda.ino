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

    // handle received data with lambda which has incoming argument types/data
    // if you want to communicate with data based on json array,
    // please do not forget to add MsgPack::arr_size_t
    MsgPacketizer::subscribe(Serial, RECV_INDEX,
        [&](const MsgPack::map_size_t& sz, const String& k1, const int v1, const String& k2, const int v2, const String& k3, const int v3) {
            if (sz.size() == 3) {  // if map size is correct
                // send same data back to app
                MsgPacketizer::send_map(Serial, SEND_INDEX, k1, v1, k2, v2, k3, v3);
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
