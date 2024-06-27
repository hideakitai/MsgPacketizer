import * as mp from "msgpacketizer";
import * as common from "./common";
import winston from "winston";
import { program } from "commander";
const { SerialPort } = require("serialport");

// parse command line arguments
program
	.argument("<port>", "serial port")
	.option("-b, --baudrate <number>", "baudrate", "115200")
	.option("-v, --verbose", "verbose mode", false)
	.parse();
const args = program.args;
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

// create serial port
const arduino = new SerialPort({
	path: args[0],
	baudRate: parseInt(options.baudrate),
	autoOpen: false,
});
arduino.on("open", (err: Error) => {
	logger.info("serial port opened");
	if (err) {
		return logger.error("failed to open serialport: ", err.message);
	}
});
arduino.on("error", (err: Error) => {
	logger.error("serial port error: ", err.message);
});
arduino.on("data", (data: Buffer) => {
	subscriber.feed(data);
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

// open serial port
arduino.open();

// send data to serial port periodically
(async () => {
	const sleep = (ms: number) =>
		new Promise((resolve) => setTimeout(resolve, ms));

	while (true) {
		try {
			if (arduino.isOpen) {
				const [simple, simple_arr, simple_map, custom] =
					common.generateCurrentData();
				arduino.write(mp.encode(common.SEND_INDEX_SIMPLE, simple));
				arduino.write(mp.encode(common.SEND_INDEX_ARR, simple_arr));
				arduino.write(mp.encode(common.SEND_INDEX_MAP, simple_map));
				arduino.write(mp.encode(common.SEND_INDEX_CUSTOM, custom));
			} else {
				logger.warn("serial port is not open");
			}
			// sleep 0.1 sec
			await sleep(100);
		} catch (error) {
			logger.error("Error:", error);
		}
	}
})();
