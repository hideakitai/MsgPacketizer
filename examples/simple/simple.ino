#include <MsgPacketizer.h>

#include <vector>
#include <map>

// input to msgpack
int i;
float f;
String s;
std::vector<int> v;
std::map<String, float> m;

uint8_t receive_index = 0x12;
uint8_t send_data_index = 0x34;
uint8_t lambda_index = 0x56;
uint8_t send_back_index = 0x78;

void setup()
{
    Serial.begin(115200);

    MsgPacketizer::subscribe(Serial, receive_index, i, f, s, v, m);

    MsgPacketizer::subscribe(Serial, lambda_index,
    [&](const int& ii, const float& ff, const String& ss, const std::vector<int>& vv, const std::map<String, float>& mm)
    {
        // send data which is updated in subscribe() above
        // MsgPacketizer::send(Serial, send_data_index, i, f, s, v, m);
        MsgPacketizer::send(Serial, send_back_index, i, f, s, v, m);
        // // send received data back
        // MsgPacketizer::send(Serial, send_back_index, ii, ff, ss, vv, mm);
        i++;
    });
}

void loop()
{
    MsgPacketizer::parse();

    MsgPacketizer::send(Serial, send_data_index, i, f, s, v, m);

    delay(10);
}
