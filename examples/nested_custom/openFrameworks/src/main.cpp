#include "ofMain.h"
#include "ofxMsgPacketizer.h"

struct Array
{
    int i; float f; std::string s;
    // json [i, f, s]
    MSGPACK_DEFINE(i, f, s);
};

struct Nested
{
    std::string key_i; int   val_i;
    std::string key_a; Array val_a;
    // json {key_i : val_i, key_a : [i, f, s]}
    MSGPACK_DEFINE_MAP(key_i, val_i, key_a, val_a);
};

class ofApp : public ofBaseApp
{
    ofSerial serial;
    string modem {"COM9"}; // <= change to your own board

    // {key_i : val_i, key_a : [i, f, s]}
    Nested nested;
    const uint8_t send_index = 0x12;
    const uint8_t recv_index = 0x34;

    stringstream echo_info;
    stringstream recv_info;

public:

    void setup()
    {
        ofSetVerticalSync(false);
        ofSetFrameRate(120);
        ofSetBackgroundColor(0);

        serial.setup(modem, 115200);

        // publish packet (you can also send function returns)
        MsgPacketizer::publish(serial, send_index, nested)
            ->setFrameRate(60);

        // handle updated data from arduino
        MsgPacketizer::subscribe(serial, recv_index,
            [&](const Nested& n)
            {
                echo_info << std::dec << std::fixed << std::setprecision(2);
                echo_info << "{ " << n.key_i << " : " << n.val_i << ", ";
                echo_info << n.key_a << " : ";
                echo_info << "[ " << n.val_a.i << ", " << n.val_a.f << ", " << n.val_a.s << " ] }";
                echo_info << std::endl;
            }
        );

        // always called if packet has come regardless of index
        MsgPacketizer::subscribe(serial, [&](const uint8_t index, MsgPack::Unpacker& unpacker)
        {
            recv_info << "packet has come! index = 0x" << std::hex << (int)index;
            recv_info << ", arg size = " << std::dec << unpacker.size() << std::endl;
        });
    }

    void update()
    {
        recv_info.str(""); recv_info.clear();
        echo_info.str(""); echo_info.clear();

        // update publishing data
        nested.key_i = "sec";
        nested.val_i = ofGetSeconds();
        nested.key_a = "time";
        nested.val_a.i = ofGetFrameNum();
        nested.val_a.f = ofGetFrameRate();
        nested.val_a.s = ofGetTimestampString("%X");

        // must be called
        MsgPacketizer::update();
    }

    void draw()
    {
        ofDrawBitmapString("FPS : " + ofToString(ofGetFrameRate()), 20, 40);
        ofDrawBitmapString(recv_info.str(), 20, 80);
        ofDrawBitmapString(echo_info.str(), 20, 120);
    }
};


int main( )
{
    ofSetupOpenGL(480, 360, OF_WINDOW);
    ofRunApp(new ofApp());
}
