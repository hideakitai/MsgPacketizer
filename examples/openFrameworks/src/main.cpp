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
    
    int io = 0;
    std::string so {"hey"};
    std::vector<int> vo {111, 222, 333};
    std::map<std::string, float> mo {{"time", 0.0}, {"fps", 0.0}};
    
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

        // publish packet (you can also send function returns)
        MsgPacketizer::publish(serial, send_direct_index, io, so, vo)
            ->setFrameRate(60);
        MsgPacketizer::publish(serial, send_lambda_index, &ofGetFrameRate, mo)
            ->setFrameRate(60);

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
        
        // update publishing data
        io = ofGetFrameNum();
        vo[1] = ofGetFrameNum();
        mo["time"] = ofGetElapsedTimef();
        mo["fps"] = ofGetFrameRate();
        
        // must be called
        MsgPacketizer::update();
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
