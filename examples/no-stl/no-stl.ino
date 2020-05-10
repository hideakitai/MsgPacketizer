#include <MsgPacketizer.h>

// for NO-STL boards:
// MsgPack::arr_t<T> = arx::vector<T>
// MsgPack::map_t<T, U> = arx::map<T, U>
// MsgPack::bin_t<T> = arx::vector<T = uint8_t || char>

// input to msgpack
int i;
float f;
String s;
MsgPack::arr_t<int> v;
MsgPack::map_t<String, float> m;

const uint8_t recv_direct_index = 0x12;
const uint8_t send_direct_index = 0x34;
const uint8_t recv_lambda_index = 0x56;
const uint8_t send_back_index = 0x78;

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    delay(2000);

    // update received data directly (up to 4 variables)
    MsgPacketizer::subscribe(Serial, recv_direct_index, i, f, s);

    // handle received data depeneding on index
    Packetizer::subscribe(Serial, recv_lambda_index, [&](const uint8_t* data, const uint8_t size)
    {
        // unpack msgpack objects
        MsgPack::Unpacker unpacker;
        unpacker.feed(data, size);
        unpacker.decode(v, m);

        // send received data back
        MsgPacketizer::send(Serial, send_back_index, v, m);
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

    static uint32_t prev_ms = millis();
    if (millis() > prev_ms + 16)
    {
        // send received data back
        MsgPacketizer::send(Serial, send_direct_index, i, f, s, v, m);
        prev_ms = millis();
    }
}
