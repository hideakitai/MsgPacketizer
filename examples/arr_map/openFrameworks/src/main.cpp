#include "ofMain.h"
#include "ofxMsgPacketizer.h"

class ofApp : public ofBaseApp
{
    ofSerial serial;
    stringstream echo_info;
    stringstream recv_info;
    string modem {"COM9"}; // <= change to your own board

    std::vector<int> v;
    std::map<std::string, int> m;

    const uint8_t send_index_arr = 0x12;
    const uint8_t recv_index_arr = 0x34;
    const uint8_t send_index_map = 0x56;
    const uint8_t recv_index_map = 0x78;

public:

    void setup()
    {
        ofSetVerticalSync(false);
        ofSetFrameRate(120);
        ofSetBackgroundColor(0);

        serial.setup(modem, 115200);

        // publish packet (you can also send function returns)
        MsgPacketizer::publish(serial, send_index_arr, v)
            ->setFrameRate(60);
        MsgPacketizer::publish(serial, send_index_map, m)
            ->setFrameRate(60);

        // handle updated data from arduino
        MsgPacketizer::subscribe(serial, recv_index_arr,
            [&](const std::vector<int>& arr)
            {
                echo_info << std::dec;
                echo_info << "arr : [ ";
                for (const auto& a : arr) echo_info << a << ", ";
                echo_info << " ]" << std::endl;
            }
        );

        // handle updated data from arduino
        MsgPacketizer::subscribe(serial, recv_index_map,
            [&](const std::map<std::string, int>& mp)
            {
                echo_info << std::dec;
                echo_info << "map : { ";
                for (const auto& m : mp)
                    echo_info << m.first << " : " << m.second << ", ";
                echo_info << " }" << std::endl;
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

        v.clear();
        v.emplace_back((int)ofGetElapsedTimef());
        v.emplace_back((int)ofGetElapsedTimeMillis());
        v.emplace_back((int)ofGetElapsedTimeMicros());

        m["frame"] = ofGetFrameNum();
        m["time"] = (int)ofGetElapsedTimef();

        // must be called
        MsgPacketizer::update();
    }

    void draw()
    {
        ofDrawBitmapString("FPS : " + ofToString(ofGetFrameRate()), 20, 40);
        ofDrawBitmapString(recv_info.str(), 20, 80);
        ofDrawBitmapString(echo_info.str(), 20, 140);
    }
};


int main( )
{
    ofSetupOpenGL(480, 360, OF_WINDOW);
    ofRunApp(new ofApp());
}
