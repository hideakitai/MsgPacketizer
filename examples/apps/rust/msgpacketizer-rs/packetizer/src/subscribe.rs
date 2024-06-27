use crate::common::MessageRef;
use crate::decode::decode_in_place;
use log::{debug, error, warn};
use std::collections::HashMap;
use std::sync::{Arc, Mutex};

type Callback = dyn Fn(MessageRef) + Send + Sync;

pub struct Subscriber<const N: usize> {
    buffer: [u8; N],
    len: usize,
    callbacks: HashMap<u8, Arc<Mutex<Callback>>>,
}

impl<const N: usize> Subscriber<N> {
    pub fn new() -> Self {
        Self {
            buffer: [0u8; N],
            len: 0,
            callbacks: HashMap::new(),
        }
    }

    pub fn feed(&mut self, input: &[u8]) -> usize {
        let len = self.len + input.len();
        if len > N {
            error!("The length of the input exceeds the buffer size: {len} > {N}",);
            return self.len;
        }

        // Update buffer
        self.buffer[self.len..len].copy_from_slice(input);
        self.len = len;

        // Find the delimiter (0x00) and decode received buffer
        let data = self.buffer[..self.len].as_mut();
        let mut pos_begin = 0;
        while let Some(pos_delim) = data.iter().skip(pos_begin).position(|x| *x == 0x00) {
            if pos_delim == 0 {
                warn!("Data starts with 0x00 -> skip");
                break;
            }
            let pos_next = pos_begin + pos_delim + 1;
            if data.len() < pos_next {
                debug!("Incomplete data {} bytes -> skip", data.len() - pos_begin);
                break;
            }

            let chunk = &mut data[pos_begin..pos_next];
            let (msg, consumed) = decode_in_place(chunk).unwrap();
            if let Some(cb_mtx) = self.callbacks.get(&msg.index) {
                let callback = cb_mtx.lock().unwrap();
                callback(msg);
            }
            pos_begin += consumed;
        }
        // If no delimiter is found, keep the remaining data in the buffer
        if pos_begin == 0 {
            return pos_begin;
        }

        let rest_len = self.len - pos_begin;
        if rest_len > 0 {
            // Remove consumed data and copy the remaining data to the beginning of the buffer
            let (consumed, rest) = self.buffer.split_at_mut(pos_begin);
            let rest = &rest[..rest_len];
            consumed[..rest.len()].copy_from_slice(rest);
            self.len = rest_len;
        } else {
            self.len = 0;
        }
        self.len
    }

    pub fn subscribe(&mut self, index: u8, callback: Arc<Mutex<Callback>>) {
        self.callbacks.insert(index, callback);
    }

    pub fn unsubscribe(&mut self, index: u8) {
        self.callbacks.remove(&index);
    }
}

#[cfg(test)]
mod test {
    use super::*;
    use env_logger;
    use std::sync::Once;
    static INIT: Once = Once::new();

    fn setup() {
        INIT.call_once(|| {
            std::env::set_var("RUST_LOG", "debug");
            env_logger::builder().is_test(true).try_init().unwrap();
        });
    }

    #[test]
    fn test_subscribe_empty() {
        setup();

        let input = [];

        let mut subscriber = Subscriber::<1024>::new();
        subscriber.subscribe(
            0x01,
            Arc::new(Mutex::new(|_: MessageRef| {
                assert!(false); // Never come here
            })),
        );
        let remained = subscriber.feed(&input);
        assert_eq!(remained, 0);
    }

    #[test]
    fn test_subscribe_simple() {
        setup();

        let input = [
            0x22, 0x01, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
            0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x5b, 0x73, 0x65, 0x63, 0x5d, 0x43, 0x00,
        ];

        let mut subscriber = Subscriber::<1024>::new();
        subscriber.subscribe(
            0x01,
            Arc::new(Mutex::new(|msg: MessageRef| {
                assert_eq!(msg.index, 0x01);
                assert_eq!(
                    msg.data,
                    [
                        0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
                        0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37,
                        0x38, 0x39, 0x5b, 0x73, 0x65, 0x63, 0x5d,
                    ]
                );
            })),
        );
        let remained = subscriber.feed(&input);
        assert_eq!(remained, 0);
    }

    #[test]
    fn test_subscribe_simple_with_additional_byte() {
        setup();

        let input = [
            0x22, 0x01, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
            0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
            0x5b, 0x73, 0x65, 0x63, 0x5d, 0x43, 0x00, 0x22, // additional byte
        ];

        let mut subscriber = Subscriber::<1024>::new();
        subscriber.subscribe(
            0x01,
            Arc::new(Mutex::new(|msg: MessageRef| {
                assert_eq!(msg.index, 0x01);
                assert_eq!(
                    msg.data,
                    [
                        0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f,
                        0xbe, 0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37,
                        0x38, 0x39, 0x5b, 0x73, 0x65, 0x63, 0x5d,
                    ]
                );
            })),
        );
        let next_pos = subscriber.feed(&input);
        assert_eq!(next_pos, 1);

        let input = [
            // starts from 2nd byte
            0x01, 0x93, 0xce, 0x07, 0x5b, 0xcd, 0x15, 0xcb, 0x40, 0xfe, 0x24, 0x0c, 0x9f, 0xbe,
            0x76, 0xc9, 0xaf, 0x31, 0x32, 0x33, 0x2e, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x5b,
            0x73, 0x65, 0x63, 0x5d, 0x43, 0x00,
        ];
        let next_pos = subscriber.feed(&input);
        assert_eq!(next_pos, 0);
    }
}
