package common

import "time"

const SEND_INDEX_SIMPLE = 0x01
const RECV_INDEX_SIMPLE = 0x02
const SEND_INDEX_ARR = 0x11
const RECV_INDEX_ARR = 0x12
const SEND_INDEX_MAP = 0x21
const RECV_INDEX_MAP = 0x22
const SEND_INDEX_CUSTOM = 0x31
const RECV_INDEX_CUSTOM = 0x32

var _TIME_START int64 = 0

func GenerateCurrentData() ([]interface{}, []int64, map[string]int64, map[string]interface{}) {
	if _TIME_START == 0 {
		// get current time in microseconds
		_TIME_START = time.Now().UnixMicro()
	}
	micros := time.Now().UnixMicro() - _TIME_START
	millis := micros / 1000.0
	seconds := millis / 1000.0

	simple := []interface{}{micros, millis, `{seconds}[sec]`}
	simple_arr := []int64{micros, int64(millis), int64(seconds)}
	simple_map := map[string]int64{
		"micros":  micros,
		"millis":  int64(millis),
		"seconds": int64(seconds),
	}
	custom := map[string]interface{}{
		"ka": simple,
		"km": simple_map,
	}

	return simple, simple_arr, simple_map, custom

}
