package msgpacketizer

import (
	// TODO:
	// "github.com/hideakitai/packetizer-go"
	"packetizer-go"

	"github.com/sirupsen/logrus"

	"github.com/vmihailenco/msgpack/v5"
)

func Decode(data []byte) (Message, error) {
	logrus.Debugf("decode input: %s", ToHex(data))
	decoded, err := packetizer.Decode(data)
	if err != nil {
		return Message{}, err
	}
	logrus.Debugf("decoded: %s", ToHex(decoded.Data))
	var msg interface{}
	err = msgpack.Unmarshal(decoded.Data, &msg)
	if err != nil {
		logrus.Warnf("msgpack unmarshal error: %s", err)
		return Message{}, err
	}
	return Message{decoded.Index, msg}, nil
}
