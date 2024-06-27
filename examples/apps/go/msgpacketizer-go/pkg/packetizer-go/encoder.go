package packetizer

import (
	"github.com/dim13/cobs"
	"github.com/sirupsen/logrus"
)

func Encode(index uint8, data []byte) []byte {
	logrus.Debugf("encode input: index=%d, data=%s", index, ToHex(data))
	var packet []byte
	packet = append(packet, index)
	packet = append(packet, data...)
	CRC8_HASH.Reset()
	CRC8_HASH.Update(data)
	crc8 := CRC8_HASH.CRC8()
	packet = append(packet, crc8)
	encoded := cobs.Encode(packet)
	logrus.Debugf("encoded: %s", ToHex(encoded))
	return append(encoded, COBS_DELIMITER)
}
