#include <MsgPacketizer.h>

// msgpac input && output
int i = 1; float f = 2.2; MsgPack::str_t s = "3.3";

const uint8_t recv_index = 0x12;
const uint8_t send_index = 0x34;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    // update received data directly
    MsgPacketizer::subscribe_arr(Serial, recv_index, i, f, s);

    // register variables to publish repeatedly
    MsgPacketizer::publish_arr(Serial, send_index, i, f, s)
        ->setFrameRate(120); // and you can change publish cycles
        // ->setIntervalSec(1.f); // also you can set interval as sec, msec, usec

    // you can subscribe values like this for more flexible way
    //
    // this is same as above
    // MsgPack::arr_size_t t; // but this must NOT be local
    // MsgPacketizer::subscribe(Serial, recv_index, t, i, f, s);
    //
    // this subscribe two arrays : [[i, f, s], [i, f, s]]
    // MsgPack::arr_size_t t, tt; // this must not be local
    // MsgPacketizer::subscribe(Serial, recv_index,
    //     t, i, s, v,
    //     tt, ii, ff, ss
    // );
    //
    // you can also publish like this for more flexible way
    //
    // this is same as above
    // MsgPacketizer::publish(Serial, send_index, MsgPack::arr_size_t(3), i, f, s);
    //
    // this publishes two arrays : [[i, f, s], [ii, ff, ss]]
    // MsgPacketizer::publish(Serial, send_index,
    //     MsgPack::arr_size_t(3), i, f, s,
    //     MsgPack::arr_size_t(3), ii, ff, ss
    // );
}

void loop()
{
    // must be called to trigger callback and publish data
    MsgPacketizer::update();

    // or you can call parse() and post() separately
    // MsgPacketizer::parse(); // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
