package msgpacketizer

// TODO:
// import "github.com/hideakitai/packetizer-go"
import "packetizer-go"

type Message struct {
	Index uint8
	Msgs  interface{}
}

func ToHex(data []byte) string {
	return packetizer.ToHex(data)
}
