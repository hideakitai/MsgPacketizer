#include <MsgPacketizer.h>

const uint8_t recv_index = 0x12;
const uint8_t send_index = 0x34;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // handle received data with lambda which has incoming argument types/data
    // if you want to communicate with data based on json array,
    // please do not forget to add MsgPack::arr_size_t
    MsgPacketizer::subscribe(Serial, recv_index,
        [&](const MsgPack::arr_size_t& sz, const int i, const float f, const String& s)
        {
            if (sz.size() == 3) // if array size is correct
            {
                // modify input and send back as array format
                static int count = 0;
                String str = s + " " + sz.size() + " " + count++;
                MsgPacketizer::send_arr(Serial, send_index, i * 2, f * 2, str);
            }
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
