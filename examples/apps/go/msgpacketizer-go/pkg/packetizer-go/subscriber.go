package packetizer

import (
	"bytes"

	"github.com/sirupsen/logrus"
)

type Subscriber struct {
	Buffer    []byte
	Msgs      []Message
	Callbacks map[uint8]func(Message)
}

func NewSubscriber() *Subscriber {
	return &Subscriber{
		Buffer:    make([]byte, 0),
		Msgs:      make([]Message, 0),
		Callbacks: map[uint8]func(Message){},
	}
}

func (s *Subscriber) Feed(buffer []byte) {
	logrus.Debugf("feed input: %s", ToHex(buffer))

	if len(buffer) == 0 {
		return
	}

	s.Buffer = append(s.Buffer, buffer...)

	for {
		// find delimiter
		i := bytes.IndexByte(s.Buffer, COBS_DELIMITER)
		if i < 0 {
			break
		}

		// decode packet
		chunk := s.Buffer[:i]
		s.Buffer = s.Buffer[i+1:]
		msg, err := Decode(chunk)
		if err != nil {
			logrus.Warnf("decode error: %s", err)
			continue
		}

		// append to msgs
		s.Msgs = append(s.Msgs, msg)
	}

	// iterate over received messages and call callbacks
	for _, msg := range s.Msgs {
		if callback, ok := s.Callbacks[msg.Index]; ok {
			callback(msg)
		}
	}

	// clear received messages
	s.Msgs = make([]Message, 0)
}

func (s *Subscriber) Subscribe(index uint8, callback func(Message)) {
	s.Callbacks[index] = callback
}

func (s *Subscriber) Unsubscribe(index uint8) {
	delete(s.Callbacks, index)
}
