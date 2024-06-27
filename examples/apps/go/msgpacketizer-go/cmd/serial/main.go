package main

import (
	"flag"
	"log"
	"time"

	"internal/common"

	"github.com/sirupsen/logrus"
	"go.bug.st/serial"

	// TODO:
	// "github.com/hideakitai/msgpacketizer-go"
	"msgpacketizer-go"
)

func main() {
	// parse command line arguments
	var (
		baudrate = flag.Int("b", 115200, "baudrate")
		verbose  = flag.Bool("v", false, "verbose mode")
	)
	flag.Parse()
	if flag.NArg() < 1 {
		log.Fatal("Please specify serial serial port name for the first argument")
	}
	port := flag.Arg(0)

	// configure logrus based on verbose mode
	if *verbose {
		logrus.SetLevel(logrus.DebugLevel)
	}
	logrus.Debugf("port = %s", port)
	logrus.Debugf("baudrate = %d", *baudrate)
	logrus.Debugf("verbose = %t", *verbose)

	// create and open serial port
	mode := &serial.Mode{BaudRate: *baudrate}
	arduino, err := serial.Open(port, mode)
	if err != nil {
		log.Fatal(err)
	} else {
		logrus.Infof("serial port opened")
	}
	defer arduino.Close()

	// create subscriber
	subscriber := msgpacketizer.NewSubscriber()
	// subscribe messages
	subscriber.Subscribe(
		common.RECV_INDEX_SIMPLE,
		func(msg msgpacketizer.Message) {
			logrus.Infof("[simple] index = %X, msg = %+v", msg.Index, msg.Msgs)
		},
	)
	subscriber.Subscribe(
		common.RECV_INDEX_ARR,
		func(msg msgpacketizer.Message) {
			logrus.Infof("[simple_arr] index = %X, msg = %+v", msg.Index, msg.Msgs)
		},
	)
	subscriber.Subscribe(
		common.RECV_INDEX_MAP,
		func(msg msgpacketizer.Message) {
			logrus.Infof("[simple_map] index = %X, msg = %+v", msg.Index, msg.Msgs)
		},
	)
	subscriber.Subscribe(
		common.RECV_INDEX_CUSTOM,
		func(msg msgpacketizer.Message) {
			logrus.Infof("[custom] index = %X, msg = %+v", msg.Index, msg.Msgs)
		},
	)

	// read
	go func() {
		for {
			buffer := make([]byte, 512)
			// NOTE: sometimes packet drop occurs..? (69 bytes as usual, sometimes 18 bytes)
			n, err := arduino.Read(buffer)
			if err != nil {
				logrus.Errorf("arduino.Read(): %s", err)
			} else if n > 0 {
				logrus.Debugf("read %d bytes, data = %s", n, msgpacketizer.ToHex(buffer[:n]))
				subscriber.Feed(buffer[:n])
			}
		}
	}()

	// write
	for {
		simple, simple_arr, simple_map, custom := common.GenerateCurrentData()

		simple_encoded, err := msgpacketizer.Encode(
			common.SEND_INDEX_SIMPLE,
			simple,
		)
		if err != nil {
			log.Fatal(err)
		}
		_, err = arduino.Write(simple_encoded)
		if err != nil {
			log.Fatal(err)
		}

		simple_arr_encoded, err := msgpacketizer.Encode(
			common.SEND_INDEX_ARR,
			simple_arr,
		)
		if err != nil {
			log.Fatal(err)
		}
		_, err = arduino.Write(simple_arr_encoded)
		if err != nil {
			log.Fatal(err)
		}

		simple_map_encoded, err := msgpacketizer.Encode(
			common.SEND_INDEX_MAP,
			simple_map,
		)
		if err != nil {
			log.Fatal(err)
		}
		_, err = arduino.Write(simple_map_encoded)
		if err != nil {
			log.Fatal(err)
		}

		custom_encoded, err := msgpacketizer.Encode(
			common.SEND_INDEX_CUSTOM,
			custom,
		)
		if err != nil {
			log.Fatal(err)
		}
		_, err = arduino.Write(custom_encoded)
		if err != nil {
			log.Fatal(err)
		}

		time.Sleep(100 * time.Millisecond)
	}
}
