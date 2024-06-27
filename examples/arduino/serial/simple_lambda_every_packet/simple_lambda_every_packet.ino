// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>

const uint8_t RECV_INDEX = 0x01;
const uint8_t SEND_INDEX = 0x02;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // callback which is always called when packet has come
    MsgPacketizer::subscribe(Serial,
        [&](const uint8_t index, MsgPack::Unpacker& unpacker) {
            if (index != RECV_INDEX) {
                return;
            }

            // manually deserialize packet
            MsgPack::arr_size_t sz;
            int i = 0;
            float f = 0.f;
            String s = "";
            unpacker.deserialize(sz, i, f, s);

            // send same data back to app
            MsgPacketizer::send(Serial, SEND_INDEX, sz, i, f, s);
        });
}

void loop() {
    // must be called to trigger callback and publish data
    // MsgPacketizer::update();

    // or you can call parse() and post() separately
    MsgPacketizer::parse();  // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
