#include <MsgPacketizer.h>

struct Array
{
    int i; float f; String s;
    // json [i, f, s]
    MSGPACK_DEFINE(i, f, s);
};

struct Nested
{
    String key_i; int   val_i;
    String key_a; Array val_a;
    // json {key_i : val_i, key_a : [i, f, s]}
    MSGPACK_DEFINE_MAP(key_i, val_i, key_a, val_a);
};

Nested nested;
const uint8_t recv_index = 0x12;
const uint8_t send_index = 0x34;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // handle received data with lambda which has incoming argument types/data
    MsgPacketizer::subscribe(Serial, recv_index,
        [&](const Nested& n)
        {
            // update global variable "nested"
            nested.key_i = n.key_i;
            nested.val_i = n.val_i;
            nested.key_a = n.key_a;
            nested.val_a.i = n.val_a.i;
            nested.val_a.f = n.val_a.f;
            nested.val_a.s = n.val_a.s;
        }
    );

    // publish nested value
    MsgPacketizer::publish(Serial, send_index, nested)
        ->setFrameRate(120.f);
}

void loop()
{
    // must be called to trigger callback and publish data
    MsgPacketizer::update();
}
