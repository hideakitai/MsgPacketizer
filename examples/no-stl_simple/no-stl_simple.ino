#include <MsgPacketizer.h>

// for NO-STL boards:
// MsgPack::arr_t -> arx::vector
// MsgPack::map_t -> arx::map
// MsgPack::bin_t -> arx::vector<uint8_t> or <char>

// input to msgpack
int i;
float f;
String s;
MsgPack::arr_t<int> v;
MsgPack::map_t<String, float> m;

uint8_t recv_direct_index = 0x12;
uint8_t send_direct_index = 0x34;
uint8_t lambda_index = 0x56;
uint8_t send_back_index = 0x78;

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    delay(2000);

    // update received data directly (up to 4 variables)
    MsgPacketizer::subscribe(Serial, recv_direct_index, i, f, s, v);
}

void loop()
{
    MsgPacketizer::parse(); // must be called to trigger callback

    // send received data back
    MsgPacketizer::send(Serial, send_back_index, i, f, s, v, m);
}
