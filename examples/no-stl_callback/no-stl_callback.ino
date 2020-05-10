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

    // handle received data depeneding on index
    Packetizer::subscribe(Serial, lambda_index, [&](const uint8_t* data, const uint8_t size)
    {
        // unpack msgpack objects
        MsgPack::Unpacker unpacker;
        unpacker.feed(data, size);
        unpacker.decode(i, f, s, v, m);

        // send received data back
        MsgPacketizer::send(Serial, send_back_index, i, f, s, v, m);
    });

    // callback which is always called when packet has come
    Packetizer::subscribe(Serial, [&](const uint8_t index, const uint8_t* data, const uint8_t size)
    {
        static bool b = false;
        digitalWrite(LED_BUILTIN, b);
        b = !b;

        (void)index; (void)data; (void)size; // just avoid warning
    });
}

void loop()
{
    MsgPacketizer::parse(); // must be called to trigger callback
}
