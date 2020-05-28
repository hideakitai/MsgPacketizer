#include <MsgPacketizer.h>

// for STL enabled boards:
// MsgPack::arr_t<T> = std::vector<T>
// MsgPack::map_t<T, U> = std::map<T, U>
// MsgPack::bin_t<T> = std::vector<T = uint8_t || char>

// for NO-STL boards(AVR, megaAVR, SAMD, SPRESENSE):
// MsgPack::arr_t<T> = arx::vector<T>
// MsgPack::map_t<T, U> = arx::map<T, U>
// MsgPack::bin_t<T> = arx::vector<T = uint8_t || char>

// input to msgpack
int i;
float f;
MsgPack::str_t s;
MsgPack::arr_t<int> v;
MsgPack::map_t<String, float> m;

const uint8_t recv_direct_index = 0x12;
const uint8_t recv_lambda_index = 0x34;
const uint8_t send_index = 0x56;

void setup()
{
    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);
    delay(2000);

    // register variables to publish repeatedly
    MsgPacketizer::publish(Serial, send_index, i, f, s, v, m)
        ->setFrameRate(60); // and you can change publish cycles
        // ->setIntervalSec(1.f); // also you can set interval as sec, msec, usec

    // update received data directly
    MsgPacketizer::subscribe(Serial, recv_direct_index, i, s, v);

    // handle received data with lambda which has incoming argument types/data
    MsgPacketizer::subscribe(Serial, recv_lambda_index,
        [&](const float& ff, const MsgPack::map_t<MsgPack::str_t, float>& mm)
        {
            f = ff + 100.f;
            m = mm;
        }
    );

    // callback which is always called when packet has come
    MsgPacketizer::subscribe(Serial,
        [&](const uint8_t index, MsgPack::Unpacker& unpacker)
        {
            // // you can also send data in one-line
            // MsgPacketizer::send(Serial, send_index, i, f, s, v, m);

            // indicate by led
            static bool b = false;
            digitalWrite(LED_BUILTIN, b);
            b = !b;
        }
    );
}

void loop()
{
    // must be called to trigger callback and publish data
    MsgPacketizer::update();

    // or you can call parse() and post() separately
    // MsgPacketizer::parse(); // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
