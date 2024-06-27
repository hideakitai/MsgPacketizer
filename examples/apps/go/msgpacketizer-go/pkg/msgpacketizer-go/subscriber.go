package msgpacketizer

import (
	// TODO:
	// "github.com/hideakitai/packetizer-go"
	"packetizer-go"

	"github.com/sirupsen/logrus"
	"github.com/vmihailenco/msgpack/v5"
)

type Subscriber struct {
	subscriber *packetizer.Subscriber
	callbacks  map[uint8]func(Message)
}

func NewSubscriber() *Subscriber {
	return &Subscriber{
		subscriber: packetizer.NewSubscriber(),
		callbacks:  map[uint8]func(Message){},
	}
}

func (s *Subscriber) Feed(buffer []byte) {
	s.subscriber.Feed(buffer)
}

func (s *Subscriber) Subscribe(index uint8, callback func(Message)) {
	s.callbacks[index] = callback
	s.subscriber.Subscribe(index, func(m packetizer.Message) {
		logrus.Debugf("callback input index=%d, data=%s", m.Index, ToHex(m.Data))
		var unpacked interface{}
		err := msgpack.Unmarshal(m.Data, &unpacked)
		if err != nil {
			logrus.Warnf("msgpack unmarshal error: %s", err)
			return
		}
		logrus.Debugf("unpacked: %+v", unpacked)
		msg := Message{m.Index, unpacked}

		if callback, ok := s.callbacks[msg.Index]; ok {
			callback(msg)
		} else {
			logrus.Warnf("callback not found for index=%d", msg.Index)
		}
	})
}

func (s *Subscriber) Unsubscribe(index uint8) {
	delete(s.callbacks, index)
	s.subscriber.Unsubscribe(index)
}
