#include "ofMain.h"
#include "MsgPacketizer.h"

class ofApp : public ofBaseApp
{
    ofSerial serial;
    stringstream echo_back_info;
    stringstream update_info;
    stringstream always_info;
//    string modem {"/dev/tty.usbmodem141401"}; // <= change to your own board
    string modem {"/dev/tty.usbmodem70085801"}; // <= change to your own board

    int i;
    float f;
    std::string s;
    std::vector<int> v;
    std::map<std::string, float> m;

    const uint8_t send_direct_index = 0x12;
    const uint8_t recv_direct_index = 0x34;
    const uint8_t send_lambda_index = 0x56;
    const uint8_t recv_back_index = 0x78;

public:

    void setup()
    {
        ofSetVerticalSync(false);
        ofSetFrameRate(60);
        ofSetBackgroundColor(0);

        serial.setup(modem, 115200);

        // updated in micro controller
        MsgPacketizer::subscribe(serial, recv_direct_index, i, f, s, v, m);

        // echo back
        MsgPacketizer::subscribe(serial, recv_back_index, [&](int ii, float ff, std::string ss, std::vector<int> vv, std::map<std::string, float> mm)
        {
            echo_back_info << std::dec;
            echo_back_info << "data : " << ii << ", " << ff << ", " << ss << ", {";
            for (auto& vi : vv) echo_back_info << vi << ",";
            echo_back_info << "}, {";
            for (auto& mi : mm) echo_back_info << "{" << mi.first << "," << mi.second << "},";
            echo_back_info << "}" << std::endl;
        });

       MsgPacketizer::subscribe(serial, [&](const uint8_t index, MsgPack::Unpacker& unpacker)
       {
           always_info << "packet has come! index = 0x" << std::hex << (int)index << endl;
       });
    }

    void update()
    {
        always_info.str(""); always_info.clear();
        update_info.str(""); update_info.clear();
        echo_back_info.str(""); echo_back_info.clear();

        std::vector<int> vo {1, 2, 3, 4, 5};
        std::map<std::string, float> mo {{"one", 1.1f}, {"two", 2.2f}, {"three", 3.3f}, {"four", 4.4f}, {"five", 5.5f}};

        MsgPacketizer::parse();
        // this packet will be sent back to index = recv_direct_index
        MsgPacketizer::send(serial, send_direct_index, (int32_t)ofGetFrameNum(), 2.2f, "three", vo, mo);
        // this packet will be sent back to index = recv_back_index
        MsgPacketizer::send(serial, send_lambda_index, 1, ofGetFrameRate(), "three", vo, mo);

        update_info << std::dec;
        update_info << "data : " << i << ", " << f << ", " << s << ", {";
        for (auto& vi : v) update_info << vi << ",";
        update_info << "}, {";
        for (auto& mi : m) update_info << "{" << mi.first << "," << mi.second << "},";
        update_info << "}" << std::endl;
    }

    void draw()
    {
        ofDrawBitmapString("FPS : " + ofToString(ofGetFrameRate()), 20, 20);

        ofDrawBitmapString(always_info.str(), 20, 40);
        ofDrawBitmapString(update_info.str(), 20, 80);
        ofDrawBitmapString(echo_back_info.str(), 20, 100);

    }
};


int main( )
{
    ofSetupOpenGL(960,240,OF_WINDOW);            // <-------- setup the GL context
    ofRunApp(new ofApp());
}
