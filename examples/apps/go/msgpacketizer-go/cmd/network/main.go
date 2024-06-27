package main

import (
	"flag"
	"log"
	"net"
	"strconv"
	"time"

	"internal/common"

	"github.com/sirupsen/logrus"

	// TODO:
	// "github.com/hideakitai/msgpacketizer-go"
	"msgpacketizer-go"
)

func main() {
	// parse command line arguments
	var (
		remote_ip   = flag.String("remote-ip", "192.168.0.201", "remote server ip")
		remote_port = flag.Int("remote-port", 55555, "remote server port")
		local_port  = flag.Int("local-port", 54321, "local port to listen")
		is_tcp      = flag.Bool("tcp", false, "use tcp instead of udp")
		verbose     = flag.Bool("v", false, "verbose mode")
	)
	flag.Parse()

	// configure logrus based on verbose mode
	if *verbose {
		logrus.SetLevel(logrus.DebugLevel)
	}

	// create udp / tcp client
	var (
		conn        net.Conn
		err         error
		remote_addr string = *remote_ip + ":" + strconv.Itoa(*remote_port)
	)
	if *is_tcp {
		conn, err = net.Dial("tcp", remote_addr)
	} else {
		conn, err = net.Dial("udp", remote_addr)
	}
	if err != nil {
		log.Fatal(err)
	}
	defer conn.Close()

	// create udp server if needed
	var listener *net.UDPConn = nil
	if !*is_tcp {
		// create udp server
		local_addr, err := net.ResolveUDPAddr("udp", ":"+strconv.Itoa(*local_port))
		if err != nil {
			log.Fatal(err)
		}
		listener, err = net.ListenUDP("udp", local_addr)
		if err != nil {
			log.Fatal(err)
		}
		defer listener.Close()
	}

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
			var (
				n   int
				err error
			)
			if *is_tcp {
				n, err = conn.Read(buffer)
			} else {
				n, err = listener.Read(buffer)
			}
			if err != nil {
				log.Fatal(err)
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
		_, err = conn.Write(simple_encoded)
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
		_, err = conn.Write(simple_arr_encoded)
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
		_, err = conn.Write(simple_map_encoded)
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
		_, err = conn.Write(custom_encoded)
		if err != nil {
			log.Fatal(err)
		}

		time.Sleep(100 * time.Millisecond)
	}
}
