#include "ofMain.h"
#include "MsgPacketizer.h"

class ofApp : public ofBaseApp
{
    ofSerial serial;
    stringstream echo_back_info;
    stringstream always_info;
    string modem {"/dev/tty.usbmodem144101"}; // <= change to your own board
    
    int i;
    float f;
    std::string s;
    std::vector<int> v;
    std::map<std::string, float> m;

    const uint8_t send_direct_index = 0x12;
    const uint8_t send_lambda_index = 0x34;
    const uint8_t recv_index = 0x56;

public:

    void setup()
    {
        ofSetVerticalSync(false);
        ofSetFrameRate(60);
        ofSetBackgroundColor(0);

        serial.setup(modem, 115200);

        // handle updated data from arduino
        MsgPacketizer::subscribe(serial, recv_index,
            [&](int ii, float ff, std::string ss, std::vector<int> vv, std::map<std::string, float> mm)
            {
                i = ii; f = ff; s = ss; v = vv; m = mm;
            
                echo_back_info << std::dec << std::fixed << std::setprecision(2);
                echo_back_info << "data : " << i << ", " << f << ", " << s << ", {";
                for (auto& vi : v) echo_back_info << vi << ",";
                echo_back_info << "}, {";
                for (auto& mi : m) echo_back_info << "{" << mi.first << "," << mi.second << "},";
                echo_back_info << "}" << std::endl;
            }
        );
        // or update directly
        // MsgPacketizer::subscribe(serial, recv_index, i, f, s, v, m);
        
        // always called if packet has come regardless of index
        MsgPacketizer::subscribe(serial, [&](const uint8_t index, MsgPack::Unpacker& unpacker)
        {
            always_info << "packet has come! index = 0x" << std::hex << (int)index << endl;
        });
    }

    void update()
    {
        always_info.str(""); always_info.clear();
        echo_back_info.str(""); echo_back_info.clear();

        std::vector<int> vo {111, (int32_t)ofGetFrameNum(), 333};
        std::map<std::string, float> mo {{"one", 1.1f}, {"two", 2.2f}, {"fps", ofGetFrameRate()}};
        
        // must be called
        MsgPacketizer::parse();
        
        // send packet
        MsgPacketizer::send(serial, send_direct_index, (int32_t)ofGetFrameNum(), "hey", vo);
        MsgPacketizer::send(serial, send_lambda_index, (float)ofGetFrameRate(), mo);
    }

    void draw()
    {
        ofDrawBitmapString("FPS : " + ofToString(ofGetFrameRate()), 20, 20);

        ofDrawBitmapString(always_info.str(), 20, 60);
        ofDrawBitmapString(echo_back_info.str(), 20, 120);

    }
};


int main( )
{
    ofSetupOpenGL(800, 200, OF_WINDOW);
    ofRunApp(new ofApp());
}
