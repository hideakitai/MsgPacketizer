package msgpacketizer

import (
	// TODO:
	// "github.com/hideakitai/packetizer-go"
	"packetizer-go"

	"github.com/sirupsen/logrus"

	"github.com/vmihailenco/msgpack/v5"
)

func Encode(index uint8, msgs interface{}) ([]byte, error) {
	logrus.Debugf("encode input: index=%d, msgs=%+v", index, msgs)
	packed, err := msgpack.Marshal(&msgs)
	if err != nil {
		logrus.Warnf("msgpack marshal error: %s", err)
		return []byte{}, err
	}
	logrus.Debugf("packed: %s", ToHex(packed))
	encoded := packetizer.Encode(index, packed)
	logrus.Debugf("encoded: %s", ToHex(encoded))
	return encoded, nil
}
