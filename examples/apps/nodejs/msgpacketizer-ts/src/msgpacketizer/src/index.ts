import * as pk from "packetizer";
import { to_hex } from "packetizer";
import { pack, unpack } from "msgpackr";
import winston from "winston";

let logger: winston.Logger | undefined = undefined;

type Callback = (msg: Message) => void;

export interface Message {
	index: number;
	msg: object;
}

export { to_hex } from "packetizer";

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

export function encode(index: number, data: object): Uint8Array {
	logDebug(`encode input: ${index}, ${data}`);
	const msg = pack(data);
	logDebug(`packed: ${to_hex(msg)}`);
	const encoded = pk.encode(index, msg);
	logDebug(`encoded: ${to_hex(encoded)}`);
	return encoded;
}

export function decode(data: Uint8Array): Message | undefined {
	logDebug(`decode input: ${to_hex(data)}`);
	const decoded = pk.decode(data);
	if (decoded === undefined) {
		return undefined;
	}
	logDebug(`decoded: ${to_hex(decoded.data)}`);
	const index = decoded.index;
	const msg = unpack(decoded.data);
	logDebug(`unpacked: ${msg}`);
	return { index, msg };
}

export class Subscriber {
	private subscriber: pk.Subscriber;
	private callbacks: Map<number, Callback>;

	constructor() {
		this.subscriber = new pk.Subscriber();
		this.callbacks = new Map();
	}

	feed(data: Uint8Array) {
		this.subscriber.feed(data);
	}

	subscribe(index: number, callback: Callback) {
		this.callbacks.set(index, callback);
		this.subscriber.subscribe(index, (m: pk.Message) => {
			logDebug(`callback input index: ${m.index}, data: ${to_hex(m.data)}`);
			const msg: object = unpack(m.data);
			logDebug(`unpacked: ${msg}`);

			if (this.callbacks.has(index)) {
				const callback = this.callbacks.get(index);
				if (callback !== undefined) {
					callback({ index, msg });
				} else {
					logWarn(`callback for index ${index} is undefined`);
				}
			} else {
				logWarn(`callback for index ${index} not found`);
			}
		});
	}

	unsubscribe(index: number) {
		this.callbacks.delete(index);
		this.subscriber.unsubscribe(index);
	}
}
