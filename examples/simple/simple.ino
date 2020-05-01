#include <MsgPacketizer.h>
#include <vector>
#include <map>

// input to msgpack
int i;
float f;
String s;
std::vector<int> v;
std::map<String, float> m;

uint8_t recv_direct_index = 0x12;
uint8_t send_direct_index = 0x34;
uint8_t lambda_index = 0x56;
uint8_t send_back_index = 0x78;

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    // update received data directly
    MsgPacketizer::subscribe(Serial, recv_direct_index, i, f, s, v, m);

    // handle received data with lambda which has incoming argument types/data
    MsgPacketizer::subscribe(Serial, lambda_index,
    [&](const int& ii, const float& ff, const String& ss, const std::vector<int>& vv, const std::map<String, float>& mm)
    {
        // send data which is directly updated
        MsgPacketizer::send(Serial, send_direct_index, i, f, s, v, m);
        // send received data back
        MsgPacketizer::send(Serial, send_back_index, ii, ff, ss, vv, mm);
    });

    // callback which is always called when packet has come
    MsgPacketizer::subscribe(Serial, [&](const uint8_t index, MsgUnpacker& unpacker)
    {
        static bool b = false;
        digitalWrite(LED_BUILTIN, b);
        b = !b;
    });
}

void loop()
{
    MsgPacketizer::parse(); // must be called to trigger callback
}
