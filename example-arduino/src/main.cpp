#include <MsgPacketizer.h>
#include <msgpack.hpp>
#include <Arduino.h>

struct Message
{
    int id;
    float time;
    MSGPACK_DEFINE(id, time);
};

MsgPacketizer::Packer packer;
MsgPacketizer::Unpacker unpacker;

int main()
{
    Serial.begin(115200);

    while (true)
    {
        while (const int size = Serial.available())
        {
            uint8_t data[size];
            Serial.readBytes((char*)data, size);
            unpacker.feed(data, size);

            while (unpacker.available())
            {
                switch(unpacker.index())
                {
                    case 0:
                    {
                        int id;
                        unpacker >> id;
                        // or
                        // int id = unpacker.deserialize<int>();

                        // return same message
                        packer.pack(id, unpacker.index());
                        break;
                    }
                    case 1:
                    {
                        float time;
                        unpacker >> time;
                        // or
                        // float time = unpacker.deserialize<float>();

                        // return same message
                        packer.pack(time, unpacker.index());
                        break;
                    }
                    case 2:
                    {
                        Message m;
                        unpacker >> m;
                        // or
                        // Message m = unpacker.deserialize<Message>();

                        // return same message
                        packer.pack(m, unpacker.index());
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
                Serial.write(packer.data(), packer.size());
                unpacker.pop();
            }
        }
    }
}