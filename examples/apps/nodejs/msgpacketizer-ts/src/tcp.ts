import * as mp from "msgpacketizer";
import * as common from "./common";
import winston from "winston";
import { program } from "commander";
import net from "net";

// parse command line arguments
program
	.option("--remote-ip <ip>", "remote server ip", "192.168.0.201")
	.option("--remote-port <port>", "remote server port", "55555")
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

// start tcp client
const client = net.connect(parseInt(options.remotePort), options.remoteIp);
client.on("connect", () => {
	logger.info("client connected to server");
});
client.on("close", () => {
	logger.info("client closed");
	client.destroy();
});
client.on("error", (err: Error) => {
	logger.error(`socket error: ${err.message}`);
	client.destroy();
});
client.on("data", (msg: Buffer) => {
	logger.debug(`client received => ${mp.to_hex(msg)}`);
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

// send data to client periodically
(async () => {
	const sleep = (ms: number) =>
		new Promise((resolve) => setTimeout(resolve, ms));

	while (true) {
		try {
			if (!client.connecting && client.writable) {
				const [simple, simple_arr, simple_map, custom] =
					common.generateCurrentData();
				client.write(mp.encode(common.SEND_INDEX_SIMPLE, simple));
				client.write(mp.encode(common.SEND_INDEX_ARR, simple_arr));
				client.write(mp.encode(common.SEND_INDEX_MAP, simple_map));
				client.write(mp.encode(common.SEND_INDEX_CUSTOM, custom));
			} else {
				logger.warn("client is not writable");
			}
			// sleep 0.1 sec
			await sleep(100);
		} catch (error) {
			logger.error(`Error: ${error}`);
		}
	}
})();
