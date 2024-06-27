package packetizer

import (
	"fmt"

	"github.com/gdbinit/crc"
)

const COBS_DELIMITER = 0x00

var CRC8_HASH = crc.NewHash(crc.CRC8)

type Message struct {
	Index uint8
	Data  []byte
}

func ToHex(data []byte) string {
	var formattedString string
	for _, b := range data {
		formattedString += fmt.Sprintf("0x%02X ", b)
	}
	return formattedString
}
