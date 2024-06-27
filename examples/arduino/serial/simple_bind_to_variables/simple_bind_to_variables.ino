// #define MSGPACKETIZER_DEBUGLOG_ENABLE
#include <MsgPacketizer.h>

// msgpack input & output
static constexpr uint8_t RECV_INDEX = 0x01;
static constexpr uint8_t SEND_INDEX = 0x02;
int us = 0;
float ms = 0.0;
MsgPack::str_t sec = "0.0 [sec]";

void setup() {
    Serial.begin(115200);
    delay(2000);

    // update received data directly
    MsgPacketizer::subscribe_arr(Serial, RECV_INDEX, us, ms, sec);

    // register variables to publish repeatedly
    MsgPacketizer::publish_arr(Serial, SEND_INDEX, us, ms, sec)
        ->setFrameRate(10);  // and you can change publish cycles
                             // ->setIntervalSec(1.f); // also you can set interval as sec, msec, usec

    // you can subscribe values like this for more flexible way
    //
    // this is same as above
    // MsgPack::arr_size_t t; // but this must NOT be local
    // MsgPacketizer::subscribe(Serial, RECV_INDEX, t, i, f, s);
    //
    // this subscribe two arrays : [[i, f, s], [i, f, s]]
    // MsgPack::arr_size_t t, tt; // this must not be local
    // MsgPacketizer::subscribe(Serial, RECV_INDEX,
    //     t, i, s, v,
    //     tt, ii, ff, ss
    // );
    //
    // you can also publish like this for more flexible way
    //
    // this is same as above
    // MsgPacketizer::publish(Serial, SEND_INDEX, MsgPack::arr_size_t(3), i, f, s);
    //
    // this publishes two arrays : [[i, f, s], [ii, ff, ss]]
    // MsgPacketizer::publish(Serial, SEND_INDEX,
    //     MsgPack::arr_size_t(3), i, f, s,
    //     MsgPack::arr_size_t(3), ii, ff, ss
    // );
}

void loop() {
    // must be called to trigger callback and publish data
    // this updates variables with received data and send updated data back to app
    MsgPacketizer::update();

    // or you can call parse() and post() separately
    // MsgPacketizer::parse(); // must be called to trigger callback
    // MsgPacketizer::post(); // must be called to publish
}
