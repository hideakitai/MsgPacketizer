use crate::common::Message;
use log::error;
use packetizer;
use serde::de::DeserializeOwned;
use std::sync::{Arc, Mutex};

type Callback<T> = dyn Fn(Message<T>) + Send + Sync;

pub struct Subscriber<const N: usize> {
    packetizer: packetizer::Subscriber<N>,
}

impl<const N: usize> Subscriber<N> {
    pub fn new() -> Self {
        Self {
            packetizer: packetizer::Subscriber::<N>::new(),
        }
    }

    pub fn feed(&mut self, input: &[u8]) -> usize {
        self.packetizer.feed(input)
    }

    pub fn subscribe<T>(&mut self, index: u8, callback: Box<Callback<T>>)
    where
        T: DeserializeOwned + 'static,
    {
        self.packetizer.subscribe(
            index,
            Arc::new(Mutex::new(move |msg: packetizer::MessageRef| {
                if let Ok(unpacked) = rmp_serde::from_slice::<T>(&msg.data) {
                    callback(Message {
                        index: msg.index,
                        data: unpacked,
                    });
                } else {
                    error!("Failed to decode msgpack");
                }
            })),
        );
    }

    pub fn unsubscribe(&mut self, index: u8) {
        self.packetizer.unsubscribe(index);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use env_logger;
    use serde::Deserialize;
    use std::sync::mpsc;
    use std::sync::Once;
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
    }

    fn setup() {
        INIT.call_once(|| {
            std::env::set_var("RUST_LOG", "debug");
            env_logger::builder().is_test(true).try_init().unwrap();
        });
    }

    #[test]
    fn test_simple() {
        setup();

        let expected = Message::<Simple> {
            index: 0x01,
            data: Simple {
                micros: 123456789,
                millis: 123456.789,
                seconds: "123.456789[sec]".to_string(),
            },
        };
        let input = [
            0x22, 0x01, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
            0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x5b, 0x73, 0x65, 0x63, 0x5d, 0x43, 0x00,
        ];

        let mut subscriber = Subscriber::<1024>::new();
        let (tx, rx) = mpsc::channel();

        subscriber.subscribe(
            0x01,
            Box::new(move |msg: Message<Simple>| {
                tx.send(msg).unwrap();
            }),
        );

        let next = subscriber.feed(&input);
        let output = rx.recv().unwrap();
        assert_eq!(next, 0);
        assert_eq!(output, expected);
    }

    #[test]
    fn test_arr() {
        setup();

        let expected = Message::<Arr> {
            index: 0x02,
            data: Arr {
                micros: 123456789,
                millis: 123456,
                seconds: 123,
            },
        };
        let input = [
            0x22, 0x02, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
            0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x5b, 0x73, 0x65, 0x63, 0x5d, 0x43, 0x00,
        ];

        let mut subscriber = Subscriber::<1024>::new();
        let (tx, rx) = mpsc::channel();

        subscriber.subscribe(
            0x02,
            Box::new(move |msg: Message<Arr>| {
                tx.send(msg).unwrap();
            }),
        );

        let next = subscriber.feed(&input);
        let output = rx.recv().unwrap();
        assert_eq!(next, 0);
        assert_eq!(output, expected);
    }

    #[test]
    fn test_map() {
        setup();

        let expected = Message::<Map> {
            index: 0x03,
            data: Map {
                micros: 123456789,
                millis: 123456,
                seconds: 123,
            },
        };
        let input = [
            0x22, 0x03, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
            0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x5b, 0x73, 0x65, 0x63, 0x5d, 0x43, 0x00,
        ];

        let mut subscriber = Subscriber::<1024>::new();
        let (tx, rx) = mpsc::channel();

        subscriber.subscribe(
            0x03,
            Box::new(move |msg: Message<Map>| {
                tx.send(msg).unwrap();
            }),
        );

        let next = subscriber.feed(&input);
        let output = rx.recv().unwrap();
        assert_eq!(next, 0);
        assert_eq!(output, expected);
    }

    #[test]
    fn test_nested() {
        setup();

        let expected = Message::<Nested> {
            index: 0x04,
            data: Nested {
                ka: (123456789, 123456.789, "123.456789[sec]".to_string()),
            },
        };
        let input = [
            0x22, 0x04, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
            0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x5b, 0x73, 0x65, 0x63, 0x5d, 0x43, 0x00,
        ];

        let mut subscriber = Subscriber::<1024>::new();
        let (tx, rx) = mpsc::channel();

        subscriber.subscribe(
            0x04,
            Box::new(move |msg: Message<Nested>| {
                tx.send(msg).unwrap();
            }),
        );

        let next = subscriber.feed(&input);
        let output = rx.recv().unwrap();
        assert_eq!(next, 0);
        assert_eq!(output, expected);
    }
}
