#include "ofMain.h"
#include "ofxMsgPacketizer.h"

class ofApp : public ofBaseApp
{
    ofSerial serial;
    string modem {"COM9"}; // <= change to your own board

    // [i, f, s]
    int i; float f; std::string s;
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
        MsgPacketizer::publish_arr(serial, send_index, i, f, s)
            ->setFrameRate(60);

        // handle updated data from arduino
        MsgPacketizer::subscribe(serial, recv_index,
            [&](MsgPack::arr_size_t& sz, const int ii, const float ff, const std::string& ss)
            {
                echo_info << std::dec << std::fixed << std::setprecision(2);
                echo_info << "arr size: " << sz.size() << ", data : " << ii << ", " << ff << ", " << ss << std::endl;
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
        i = ofGetFrameNum();
        f = ofGetFrameRate();
        s = ofGetTimestampString("%X");

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

