package packetizer

import (
	"fmt"

	"github.com/dim13/cobs"
	"github.com/sirupsen/logrus"
)

func Decode(chunk []byte) (Message, error) {
	logrus.Debugf("decode input: %s", ToHex(chunk))

	// decode cobs (NOTE: 0x00 on last byte is REQUIRED)
	decoded := cobs.Decode(chunk)

	if len(decoded) < 3 {
		return Message{}, fmt.Errorf("decoded length is too short: %d", len(decoded))
	}

	// devide into index, data and crc8
	// NOTE: decoded bytes includes zero at the end
	index := decoded[0]
	data := decoded[1 : len(decoded)-2]
	crc8 := decoded[len(decoded)-2]

	// calcurate crc8
	CRC8_HASH.Reset()
	CRC8_HASH.Update(data)
	crc8_calc := CRC8_HASH.CRC8()
	logrus.Debugf("cobs decoded to index=%d, data=%s, crc8=%X, crc8_calc=%X", index, ToHex(data), crc8, crc8_calc)

	// check crc8
	if crc8 == crc8_calc {
		return Message{index, data}, nil
	} else {
		return Message{}, fmt.Errorf("crc8 mismatch: (recv: %X, calc: %X)", crc8, crc8_calc)
	}
}
