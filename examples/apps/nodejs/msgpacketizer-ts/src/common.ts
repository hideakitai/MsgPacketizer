export const SEND_INDEX_SIMPLE = 0x01;
export const RECV_INDEX_SIMPLE = 0x02;
export const SEND_INDEX_ARR = 0x11;
export const RECV_INDEX_ARR = 0x12;
export const SEND_INDEX_MAP = 0x21;
export const RECV_INDEX_MAP = 0x22;
export const SEND_INDEX_CUSTOM = 0x31;
export const RECV_INDEX_CUSTOM = 0x32;

const TIME_START = Date.now();

export function generateCurrentData() {
	const millis = Date.now() - TIME_START;
	const micros = millis * 1000.0;
	const seconds = Math.floor(millis / 1000);

	const simple = [micros, millis, `${seconds}[sec]`];
	const simple_arr = [micros, Math.floor(millis), Math.floor(seconds)];
	const simple_map = {
		micros: micros,
		millis: Math.floor(millis),
		seconds: Math.floor(seconds),
	};
	const custom = {
		ka: simple,
		km: simple_map,
	};

	return [simple, simple_arr, simple_map, custom];
}
