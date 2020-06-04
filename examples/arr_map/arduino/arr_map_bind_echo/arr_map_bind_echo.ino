#include <MsgPacketizer.h>

// for STL enabled boards:
// MsgPack::arr_t<T> = std::vector<T>
// MsgPack::map_t<T, U> = std::map<T, U>
// MsgPack::bin_t<T> = std::vector<T = uint8_t || char>

// for NO-STL boards(AVR, megaAVR, SAMD, SPRESENSE):
// MsgPack::arr_t<T> = arx::vector<T>
// MsgPack::map_t<T, U> = arx::map<T, U>
// MsgPack::bin_t<T> = arx::vector<T = uint8_t || char>

// msgpac input && output
MsgPack::arr_t<int> a {1, 2, 3}; // json: [1, 2, 3]
MsgPack::map_t<String, int> m {{"a", 1}, {"b", 2}}; // json {{"a", 1}, {"b", 2}}

const uint8_t recv_index_arr = 0x12;
const uint8_t send_index_arr = 0x34;
const uint8_t recv_index_map = 0x56;
const uint8_t send_index_map = 0x78;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // update received data directly
    MsgPacketizer::subscribe(Serial, recv_index_arr, a);
    MsgPacketizer::subscribe(Serial, recv_index_map, m);

    // register variables to publish repeatedly
    MsgPacketizer::publish(Serial, send_index_arr, a)
        ->setFrameRate(120.f);
    MsgPacketizer::publish(Serial, send_index_map, m)
        ->setFrameRate(120.f);
}

void loop()
{
    // must be called to trigger callback and publish data
    MsgPacketizer::update();
}
