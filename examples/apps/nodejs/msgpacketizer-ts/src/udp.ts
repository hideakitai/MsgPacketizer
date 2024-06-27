import * as mp from "msgpacketizer";
import * as common from "./common";
import winston from "winston";
import { program } from "commander";
import dgram from "dgram";

// parse command line arguments
program
	.option("--remote-ip <ip>", "remote server ip", "192.168.0.201")
	.option("--remote-port <port>", "remote server port", "55555")
	.option("--local-port <port>", "local port to listen", "54321")
	.option("-v, --verbose", "verbose mode", false)
	.parse();
const options = program.opts();

// create logger based on verbose option
const logger = winston.createLogger({
	level: "info",
	format: winston.format.combine(
		winston.format.colorize(),
		winston.format.simple(),
	),
	transports: [new winston.transports.Console()],
});
if (options.verbose) {
	logger.level = "debug";
}

// create msgpacketizer.subscriber
const subscriber = new mp.Subscriber();

const socket = dgram.createSocket("udp4");
socket.on("listening", () => {
	const address = socket.address();
	logger.info(`socket listening ${address.address}:${address.port}`);
});
socket.on("error", (err: Error) => {
	logger.error(`socket error: ${err.message}`);
	socket.close();
});
socket.on("message", (msg: Buffer, rinfo: dgram.RemoteInfo) => {
	logger.debug(`msg from ${rinfo.address}:${rinfo.port} => ${mp.to_hex(msg)}}`);
	subscriber.feed(msg);
});

// subscribe messages
subscriber.subscribe(common.RECV_INDEX_SIMPLE, (msg: mp.Message) => {
	logger.info(
		`[simple]     index: ${msg.index}, msg: ${JSON.stringify(msg.msg)}`,
	);
});
subscriber.subscribe(common.RECV_INDEX_ARR, (msg: mp.Message) => {
	logger.info(
		`[simple_arr] index: ${msg.index}, msg: ${JSON.stringify(msg.msg)}`,
	);
});
subscriber.subscribe(common.RECV_INDEX_MAP, (msg: mp.Message) => {
	logger.info(
		`[simple_map] index: ${msg.index}, msg: ${JSON.stringify(msg.msg)}`,
	);
});
subscriber.subscribe(common.RECV_INDEX_CUSTOM, (msg: mp.Message) => {
	logger.info(
		`[custom]     index: ${msg.index}, msg: ${JSON.stringify(msg.msg)}`,
	);
});

// start listening local port
socket.bind(parseInt(options.localPort));

// send data to client periodically
(async () => {
	const sleep = (ms: number) =>
		new Promise((resolve) => setTimeout(resolve, ms));

	while (true) {
		try {
			const [simple, simple_arr, simple_map, custom] =
				common.generateCurrentData();

			socket.send(
				mp.encode(common.SEND_INDEX_SIMPLE, simple),
				parseInt(options.remotePort),
				options.remoteIp,
			);
			socket.send(
				mp.encode(common.SEND_INDEX_ARR, simple_arr),
				parseInt(options.remotePort),
				options.remoteIp,
			);
			socket.send(
				mp.encode(common.SEND_INDEX_MAP, simple_map),
				parseInt(options.remotePort),
				options.remoteIp,
			);
			socket.send(
				mp.encode(common.SEND_INDEX_CUSTOM, custom),
				parseInt(options.remotePort),
				options.remoteIp,
			);
			// sleep 0.1 sec
			await sleep(100);
		} catch (error) {
			logger.error(`Error: ${error}`);
		}
	}
})();
