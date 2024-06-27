use log::{debug, error};
use packetizer::{self, to_hex_str};
use rmp_serde;
use serde::Serialize;

pub fn encode<T: Serialize>(index: u8, data: T) -> Option<Vec<u8>> {
    let encoded = if let Ok(data) = rmp_serde::to_vec(&data) {
        data
    } else {
        error!("Failed to encode to msgpack");
        return None;
    };
    debug!("Encoded to msgpack: {}", to_hex_str(&encoded));
    Some(packetizer::encode(index, encoded))
}

pub fn encode_named<T: Serialize>(index: u8, data: T) -> Option<Vec<u8>> {
    let encoded = if let Ok(data) = rmp_serde::to_vec_named(&data) {
        data
    } else {
        error!("Failed to encode to msgpack");
        return None;
    };
    debug!("Encoded to msgpack: {}", to_hex_str(&encoded));
    Some(packetizer::encode(index, encoded))
}

// test
#[cfg(test)]
mod tests {
    use super::*;
    use env_logger;
    use serde::Serialize;
    use std::{collections::BTreeMap, sync::Once};
    static INIT: Once = Once::new();

    #[derive(Serialize)]
    struct Simple {
        micros: u64,
        millis: f64,
        seconds: String,
    }

    #[derive(Serialize)]
    struct Arr {
        micros: u64,
        millis: u32,
        seconds: u8,
    }

    #[derive(Serialize)]
    struct Map {
        micros: u64,
        millis: u32,
        seconds: u8,
    }

    #[derive(Serialize)]
    struct Nested {
        ka: (u64, f64, String),
        km: BTreeMap<String, u64>,
    }

    fn setup() {
        INIT.call_once(|| {
            std::env::set_var("RUST_LOG", "debug");
            env_logger::builder().is_test(true).try_init().unwrap();
        });
    }

    #[test]
    fn test_encode_simple() {
        setup();

        assert_eq!(
            encode(
                0x01,
                &Simple {
                    micros: 123456789,
                    millis: 123456.789,
                    seconds: "123.456789[sec]".to_string(),
                }
            ),
            Some(vec![
                0x22, 0x01, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
                0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
                0x5b, 0x73, 0x65, 0x63, 0x5d, 0x43, 0x00
            ])
        );
    }

    #[test]
    fn test_encode_arr() {
        setup();
        assert_eq!(
            encode(
                0x11,
                &Arr {
                    micros: 123456789,
                    millis: 123456,
                    seconds: 123,
                }
            ),
            Some(vec![
                0x09, 0x11, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xce, 0x06, 0x01, 0xe2, 0x40, 0x7b,
                0xa3, 0x00
            ])
        );
        assert_eq!(
            encode(0x11, vec![123456789, 123456, 123]),
            Some(vec![
                0x09, 0x11, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xce, 0x06, 0x01, 0xe2, 0x40, 0x7b,
                0xa3, 0x00
            ])
        );
    }

    #[test]
    fn test_encode_map() {
        setup();
        assert_eq!(
            encode_named(
                0x21,
                &Map {
                    micros: 123456789,
                    millis: 123456,
                    seconds: 123
                }
            ),
            Some(vec![
                0x17, 0x21, 0x83, 0xa6, 0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0xce, 0x07, 0x5b, 0xcd,
                0x15, 0xa6, 0x6d, 0x69, 0x6c, 0x6c, 0x69, 0x73, 0xce, 0x0e, 0x01, 0xe2, 0x40, 0xa7,
                0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x73, 0x7b, 0xeb, 0x00
            ])
        );
        assert_eq!(
            encode(
                0x21,
                BTreeMap::from([("micros", 123456789), ("millis", 123456), ("seconds", 123),])
            ),
            Some(vec![
                0x17, 0x21, 0x83, 0xa6, 0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0xce, 0x07, 0x5b, 0xcd,
                0x15, 0xa6, 0x6d, 0x69, 0x6c, 0x6c, 0x69, 0x73, 0xce, 0x0e, 0x01, 0xe2, 0x40, 0xa7,
                0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x73, 0x7b, 0xeb, 0x00
            ])
        )
    }

    #[test]
    fn test_encode_nested() {
        setup();
        assert_eq!(
            encode_named(
                0x31,
                &Nested {
                    ka: (123456789, 123456.789, "123.456789[sec]".to_string()),
                    km: BTreeMap::from([
                        (String::from("micros"), 123456789),
                        (String::from("millis"), 123456),
                        (String::from("seconds"), 123),
                    ]),
                }
            ),
            Some(vec![
                0x3d, 0x31, 0x82, 0xa2, 0x6b, 0x61, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40,
                0xfe, 0x24, 0x0c, 0x9f, 0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35,
                0x36, 0x37, 0x38, 0x39, 0x5b, 0x73, 0x65, 0x63, 0x5d, 0xa2, 0x6b, 0x6d, 0x83, 0xa6,
                0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xa6, 0x6d, 0x69,
                0x6c, 0x6c, 0x69, 0x73, 0xce, 0x0e, 0x01, 0xe2, 0x40, 0xa7, 0x73, 0x65, 0x63, 0x6f,
                0x6e, 0x64, 0x73, 0x7b, 0x5b, 0x00
            ])
        );
    }
}
