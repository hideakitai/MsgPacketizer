use crate::common::Message;
use log::error;
use packetizer;
use rmp_serde;
use serde::de::DeserializeOwned;

pub fn decode<T: DeserializeOwned>(data: &[u8]) -> Option<Message<T>> {
    let decoded = if let Some(data) = packetizer::decode(data) {
        data
    } else {
        error!("Failed to decode packet");
        return None;
    };
    let unpacked = if let Ok(data) = rmp_serde::from_slice(&decoded.data) {
        data
    } else {
        error!("Failed to decode msgpack");
        return None;
    };
    Some(Message {
        index: decoded.index,
        data: unpacked,
    })
}

pub fn decode_in_place<T: DeserializeOwned>(data: &mut [u8]) -> Option<(Message<T>, usize)> {
    let (decoded, used) = if let Some((data, used)) = packetizer::decode_in_place(data) {
        (data, used)
    } else {
        error!("Failed to decode packet");
        return None;
    };
    let unpacked = if let Ok(data) = rmp_serde::from_slice(&decoded.data) {
        data
    } else {
        error!("Failed to decode msgpack");
        return None;
    };
    Some((
        Message {
            index: decoded.index,
            data: unpacked,
        },
        used,
    ))
}

#[cfg(test)]
mod tests {
    use super::*;
    use env_logger;
    use serde::Deserialize;
    use std::{collections::BTreeMap, sync::Once};
    static INIT: Once = Once::new();

    #[derive(Debug, PartialEq, Deserialize)]
    struct Simple {
        micros: u64,
        millis: f64,
        seconds: String,
    }

    #[derive(Debug, PartialEq, Deserialize)]
    struct Arr {
        micros: u64,
        millis: u32,
        seconds: u8,
    }

    #[derive(Debug, PartialEq, Deserialize)]
    struct Map {
        micros: u64,
        millis: u32,
        seconds: u8,
    }

    #[derive(Debug, PartialEq, Deserialize)]
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
    fn test_decode_simple() {
        setup();

        let expected = Message::<Simple> {
            index: 0x01,
            data: Simple {
                micros: 123456789,
                millis: 123456.789,
                seconds: "123.456789[sec]".to_string(),
            },
        };
        let mut input = [
            0x22, 0x01, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
            0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x5b, 0x73, 0x65, 0x63, 0x5d, 0x43, 0x00,
        ];

        let output: Message<Simple> = decode(&input).unwrap();
        assert_eq!(output, expected);

        let (output, used): (Message<Simple>, usize) = decode_in_place(&mut input).unwrap();
        assert_eq!(used, input.len());
        assert_eq!(output, expected);
    }

    #[test]
    fn test_decode_arr() {
        setup();

        let expected = Message::<Arr> {
            index: 0x11,
            data: Arr {
                micros: 123456789,
                millis: 123456,
                seconds: 123,
            },
        };

        let mut input = [
            0x09, 0x11, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xce, 0x06, 0x01, 0xe2, 0x40, 0x7b,
            0xa3, 0x00,
        ];

        let output: Message<Arr> = decode(&input).unwrap();
        assert_eq!(output, expected);

        let (output, used): (Message<Arr>, usize) = decode_in_place(&mut input).unwrap();
        assert_eq!(used, input.len());
        assert_eq!(output, expected);
    }

    #[test]
    fn test_decode_map() {
        setup();

        let expected = Message::<Map> {
            index: 0x21,
            data: Map {
                micros: 123456789,
                millis: 123456,
                seconds: 123,
            },
        };

        let mut input = [
            0x17, 0x21, 0x83, 0xa6, 0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0xce, 0x07, 0x5b, 0xcd,
            0x15, 0xa6, 0x6d, 0x69, 0x6c, 0x6c, 0x69, 0x73, 0xce, 0x0e, 0x01, 0xe2, 0x40, 0xa7,
            0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x73, 0x7b, 0xeb, 0x00,
        ];

        let output: Message<Map> = decode(&input).unwrap();
        assert_eq!(output, expected);

        let (output, used): (Message<Map>, usize) = decode_in_place(&mut input).unwrap();
        assert_eq!(used, input.len());
        assert_eq!(output, expected);
    }

    #[test]
    fn test_decode_nested() {
        setup();

        let expected = Message::<Nested> {
            index: 0x31,
            data: Nested {
                ka: (123456789, 123456.789, "123.456789[sec]".to_string()),
                km: BTreeMap::from([
                    (String::from("micros"), 123456789),
                    (String::from("millis"), 123456),
                    (String::from("seconds"), 123),
                ]),
            },
        };

        let mut input = [
            0x3d, 0x31, 0x82, 0xa2, 0x6b, 0x61, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40,
            0xfe, 0x24, 0x0c, 0x9f, 0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35,
            0x36, 0x37, 0x38, 0x39, 0x5b, 0x73, 0x65, 0x63, 0x5d, 0xa2, 0x6b, 0x6d, 0x83, 0xa6,
            0x6d, 0x69, 0x63, 0x72, 0x6f, 0x73, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xa6, 0x6d, 0x69,
            0x6c, 0x6c, 0x69, 0x73, 0xce, 0x0e, 0x01, 0xe2, 0x40, 0xa7, 0x73, 0x65, 0x63, 0x6f,
            0x6e, 0x64, 0x73, 0x7b, 0x5b, 0x00,
        ];

        let output: Message<Nested> = decode(&input).unwrap();
        assert_eq!(output, expected);

        let (output, used): (Message<Nested>, usize) = decode_in_place(&mut input).unwrap();
        assert_eq!(used, input.len());
        assert_eq!(output, expected);
    }
}
