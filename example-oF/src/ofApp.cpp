#include "ofApp.h"
#include "MsgPacketizer.h"

struct Message
{
    int id;
    float time;
    MSGPACK_DEFINE(id, time);
};

MsgPacketizer::Unpacker unpacker;
MsgPacketizer::Packer packer;

ofSerial serial;
stringstream info;

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetVerticalSync(false);
    ofSetFrameRate(60);
    ofSetBackgroundColor(0);
    
    serial.setup(0, 115200);
}

//--------------------------------------------------------------
void ofApp::update()
{
    while (const size_t size = serial.available())
    {
        uint8_t serial_buffer[size];
        serial.readBytes((unsigned char*)serial_buffer, size);
        
        // feed received data to unpacker
        unpacker.feed((uint8_t*)serial_buffer, size);
        
        while (unpacker.available())
        {
            info.str("");
            info.clear();
            
            switch(unpacker.index())
            {
                case 0:
                {
                    // get message
                    int id;
                    unpacker >> id;
                    // or
                    // int id = unpacker.deserialize<int>();
                    
                    info << "id : " << id << endl;
                    break;
                }
                case 1:
                {
                    // get message
                    float time;
                    unpacker >> time;
                    // or
                    // float time = unpacker.deserialize<float>();
                    
                    info << "time : " << time << endl;
                    break;
                }
                case 2:
                {
                    // get message
                    Message m;
                    unpacker >> m;
                    // or
                    // Message m = unpacker.deserialize<Message>();
                    
                    info << "id : " << m.id << endl;
                    info << "time : " << m.time << endl;
                    break;
                }
                default:
                {
                    break;
                }
            }
            // go to next data
            unpacker.pop();
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
    ofSetColor(255);
    stringstream ss;
    ss << "0 : send [int]    id" << endl;
    ss << "1 : send [float]  time" << endl;
    ss << "2 : send [struct] id and time" << endl;
    ofDrawBitmapString(ss.str(), 20, 20);
    ofDrawBitmapString(ofToString(ofGetFrameRate()), 20, 80);
    ofDrawBitmapString(info.str(), 20, 120);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

    switch(key)
    {
        case '0':
        {
            int index = 0;
            int id = ofGetFrameNum() % 256;
            packer.pack(id, index);
            serial.writeBytes(packer.data(), packer.size());
            break;
        }
        case '1':
        {
            int index = 1;
            float time = ofGetElapsedTimef();
            packer.pack(time, index);
            serial.writeBytes(packer.data(), packer.size());
            break;
        }
        case '2':
        {
            int index = 2;
            Message msg;
            msg.id    = ofGetFrameNum() % 256;
            msg.time  = ofGetElapsedTimef();
            packer.pack(msg, index);
            serial.writeBytes(packer.data(), packer.size());
            break;
        }
        default:
        {
            break;
        }
        
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
