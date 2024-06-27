import crc8 from "crc/crc8";
import cobs from "cobs";
import winston from "winston";

const COBS_DELIMITER = 0x00;
type Callback = (msg: Message) => void;
let logger: winston.Logger | undefined = undefined;

export interface Message {
	index: number;
	data: Uint8Array;
}

export function to_hex(data: Uint8Array) {
	return Array.prototype.map
		.call(data, (byte) => byte.toString(16).padStart(2, "0"))
		.join(" ");
}

export function setLogger(my_logger: winston.Logger) {
	logger = my_logger;
}

function logDebug(msg: string) {
	if (logger !== undefined) {
		logger.debug(msg);
	}
}

function logWarn(msg: string) {
	if (logger !== undefined) {
		logger.warn(msg);
	}
}

export function encode(index: number, data: Uint8Array): Uint8Array {
	logDebug(`encode input: ${index}, ${to_hex(data)}`);
	const crc = crc8(data);
	const packet = Buffer.concat([
		Buffer.from([index]),
		data,
		Buffer.from([crc]),
	]);
	const encoded = Buffer.concat([
		cobs.encode(packet),
		Buffer.from([COBS_DELIMITER]),
	]);
	logDebug(`encoded: ${to_hex(encoded)}`);
	return encoded;
}

export function decode(encoded: Uint8Array): Message | undefined {
	logDebug(`cobs decode input: ${to_hex(encoded)}`);

	// decode cobs (NOTE: 0x00 on last byte is not required)
	const delimiter_index = encoded.indexOf(COBS_DELIMITER);
	let chunk: Uint8Array;
	if (delimiter_index !== -1) {
		chunk = encoded.subarray(0, delimiter_index);
	} else {
		chunk = encoded;
	}
	const decoded = cobs.decode(chunk);

	if (decoded.length < 3) {
		logWarn(`decoded length is too short: ${decoded.length}`);
		return undefined;
	}

	// devide msg into index, data and crc
	const index: number = decoded[0];
	const data: Uint8Array = decoded.slice(1, decoded.length - 1);
	const crc = decoded[decoded.length - 1];
	const crc_calc = crc8(data);
	logDebug(
		`cobs decoded to index: ${index}, data: ${to_hex(
			data,
		)}, crc: ${crc}, crc calc: ${crc_calc}`,
	);

	// check crc8
	if (crc !== crc_calc) {
		logWarn(`crc mismatch (recv: ${crc}, calc: ${crc_calc}`);
		return undefined;
	}

	return { index, data };
}

export class Subscriber {
	private buffer: Buffer;
	private msgs: Message[];
	private callbacks: Map<number, Callback>;

	constructor() {
		this.buffer = Buffer.alloc(0);
		this.msgs = [];
		this.callbacks = new Map();
	}

	feed(data: Uint8Array) {
		logDebug(`feed input: ${to_hex(data)}`);
		this.buffer = Buffer.concat([this.buffer, Buffer.from(data)]);

		// find delimiter (0x00)
		let delimiter_index = this.buffer.indexOf(COBS_DELIMITER);
		while (delimiter_index !== -1) {
			// if delimiter found, split and process buffer one by one
			// NOTE: requires delimiter to be at the end of packet
			const chunk = this.buffer.subarray(0, delimiter_index + 1);

			// decode packet
			const decoded = decode(chunk);
			if (decoded !== undefined) {
				this.msgs.push(decoded);
			}

			// remove processed packet from buffer
			this.buffer = this.buffer.subarray(delimiter_index + 1);
			// find next delimiter
			delimiter_index = this.buffer.indexOf(COBS_DELIMITER);
		}

		// iterate over msgs and run callback if exists
		for (const msg of this.msgs) {
			logDebug(`received msg: index: ${msg.index}, data: ${to_hex(msg.data)}`);
			// do some stuff in user-defined callback function
			if (this.callbacks.has(msg.index)) {
				const callback = this.callbacks.get(msg.index);
				if (callback !== undefined) {
					callback(msg);
				} else {
					logWarn(`callback for index ${msg.index} is undefined`);
				}
			} else {
				logWarn(`callback for index ${msg.index} not found`);
			}
		}

		// clear received msgs
		this.msgs = [];
	}

	subscribe(index: number, callback: Callback) {
		this.callbacks.set(index, callback);
	}

	unsubscribe(index: number) {
		this.callbacks.delete(index);
	}
}
