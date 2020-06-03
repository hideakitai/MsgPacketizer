#include <MsgPacketizer.h>

const uint8_t recv_index = 0x12;
const uint8_t send_index = 0x34;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // callback which is always called when packet has come
    MsgPacketizer::subscribe(Serial,
        [&](const uint8_t index, MsgPack::Unpacker& unpacker)
        {
            // input to msgpack
            MsgPack::arr_size_t sz;
            int i; float f; String s;

            // manually deserialize packet and modify
            unpacker.deserialize(sz, i, f, s);
            s = s + " " + index;

            // send back data as array manually
            MsgPacketizer::send(Serial, send_index, sz, i, f, s);
        }
    );
}

void loop()
{
    // must be called to trigger callback and publish data
    // MsgPacketizer::update();

    // or you can call parse() and post() separately
    MsgPacketizer::parse(); // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
