// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>

const uint8_t RECV_INDEX = 0x01;
const uint8_t SEND_INDEX = 0x02;

void setup() {
    Serial.begin(115200);
    delay(2000);

    // - only one interface (serial, udp, tcp, etc.) is available for manual subscription
    //   because MsgPacketizer cannot indetify which data is from which device
    // - publisher is not available for unsupported data stream (manual operation)
    MsgPacketizer::subscribe_manual(RECV_INDEX,
        [&](const MsgPack::arr_size_t& sz, const int i, const float f, const String& s) {
            if (sz.size() == 3) { // if array size is correct
                // encode your data manually and get binary packet from MsgPacketizer
                const auto& packet = MsgPacketizer::encode_arr(SEND_INDEX, i, f, s);
                // send same data back to app
                Serial.write(packet.data.data(), packet.data.size());
            }
        });
}

void loop() {
    // you should feed the received data manually to MsgPacketizer
    const size_t size = Serial.available();
    if (size) {
        uint8_t* data = new uint8_t[size];
        Serial.readBytes((char*)data, size);

        // feed your binary data to MsgPacketizer manually
        // if data has successfully received and decoded,
        // subscribed callback will be called
        MsgPacketizer::feed(data, size);

        delete[] data;
    }
}
