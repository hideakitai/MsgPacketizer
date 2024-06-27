// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>

static constexpr uint8_t RECV_INDEX = 0x01;
static constexpr uint8_t SEND_INDEX = 0x02;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // handle received data with lambda which has incoming argument types/data
    // if you want to communicate with data based on json array,
    // please do not forget to add MsgPack::arr_size_t
    MsgPacketizer::subscribe(Serial, RECV_INDEX,
        [&](const MsgPack::arr_size_t& sz, const int i, const float f, const String& s) {
            if (sz.size() == 3) {  // if array size is correct
                // send same data back to app
                MsgPacketizer::send_arr(Serial, SEND_INDEX, i, f, s);
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
