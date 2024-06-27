use clap::Parser;
use env_logger;
use log::{error, info, warn};
use msgpacketizer::Message;
use once_cell::sync::Lazy;
use serde::{Deserialize, Serialize};
use serialport;
use std::cell::RefCell;
use std::sync::Mutex;
use std::time::Instant;

const SEND_INDEX_SIMPLE: u8 = 0x01;
const RECV_INDEX_SIMPLE: u8 = 0x02;
const SEND_INDEX_ARR: u8 = 0x11;
const RECV_INDEX_ARR: u8 = 0x12;
const SEND_INDEX_MAP: u8 = 0x21;
const RECV_INDEX_MAP: u8 = 0x22;
const SEND_INDEX_CUSTOM: u8 = 0x31;
const RECV_INDEX_CUSTOM: u8 = 0x32;

#[derive(Debug, PartialEq, Serialize, Deserialize)]
struct Simple {
    micros: u64,
    millis: f64,
    seconds: String,
}

#[derive(Debug, PartialEq, Serialize, Deserialize)]
struct Arr {
    micros: u64,
    millis: u32,
    seconds: u8,
}

#[derive(Debug, PartialEq, Serialize, Deserialize)]
struct Map {
    micros: u64,
    millis: u32,
    seconds: u8,
}

#[derive(Debug, PartialEq, Serialize, Deserialize)]
struct Nested {
    ka: (u64, f64, String),
}

#[derive(Debug, Parser)]
struct Cli {
    #[arg()]
    port: String,
    #[arg(short, long, default_value = "115200")]
    baudrate: u32,
    #[arg(short, long)]
    verbose: bool,
}

fn main() {
    // parse command line arguments
    let cli = Cli::parse();
    println!("{:?}", cli);

    // create logger based on verbose mode
    let log_level = if cli.verbose {
        log::LevelFilter::Debug
    } else {
        log::LevelFilter::Info
    };
    env_logger::builder().filter(None, log_level).init();

    // create and open serial port
    let builder = serialport::new(&cli.port, cli.baudrate);
    println!("{:?}", &builder);
    let mut port = builder.open().unwrap_or_else(|e| {
        error!("Failed to open {}. Error: {}", &cli.port, e);
        panic!();
    });

    // create msgpacketizer.subscriber
    let mut subscriber = msgpacketizer::Subscriber::<1024>::new();

    // subscribe messages
    subscriber.subscribe(
        RECV_INDEX_SIMPLE,
        Box::new(|msg: Message<Simple>| {
            info!("[simple] index = {:#x}, msg = {:?}", msg.index, msg.data)
        }),
    );
    subscriber.subscribe(
        RECV_INDEX_ARR,
        Box::new(|msg: Message<Arr>| {
            info!(
                "[simple_arr] index = {:#x}, msg = {:?}",
                msg.index, msg.data
            )
        }),
    );
    subscriber.subscribe(
        RECV_INDEX_MAP,
        Box::new(|msg: Message<Map>| {
            info!(
                "[simple_map] index = {:#x}, msg = {:?}",
                msg.index, msg.data
            )
        }),
    );
    subscriber.subscribe(
        RECV_INDEX_CUSTOM,
        Box::new(|msg: Message<Nested>| {
            info!("[custom] index = {:#x}, msg = {:?}", msg.index, msg.data)
        }),
    );

    // read/write
    loop {
        // read
        info!("read");
        if let Ok(n) = port.bytes_to_read() {
            if n > 0 {
                info!("bytes_to_read = {}", n);
                let mut buf = [0; 1024];
                let n_read = port.read(&mut buf).unwrap();
                let remained = subscriber.feed(&buf[..n_read as usize]);
                if remained > 0 {
                    warn!("remained = {}", remained);
                }
            }
        }

        // write
        // NOTE: 複数回連続で write すると、500 [us] などの sleep を入れないと、書き込みエラーになる
        // NOTE: Custom { kind: TimedOut, error: "Operation timed out" }
        // NOTE: なので、パケットをまとめて、一回の write call で送信するようにする
        info!("write");
        let (simple, simple_arr, simple_map, custom) = generate_current_data();
        let mut packet = vec![];
        packet.extend(msgpacketizer::encode(SEND_INDEX_SIMPLE, simple).unwrap());
        packet.extend(msgpacketizer::encode(SEND_INDEX_ARR, simple_arr).unwrap());
        packet.extend(msgpacketizer::encode_named(SEND_INDEX_MAP, simple_map).unwrap());
        packet.extend(msgpacketizer::encode_named(SEND_INDEX_CUSTOM, custom).unwrap());
        port.write_all(&packet).unwrap();

        // sleep
        std::thread::sleep(std::time::Duration::from_millis(100));
    }
}

fn generate_current_data() -> (Simple, Arr, Map, Nested) {
    // get the seconds since the program started
    static START: Lazy<Mutex<RefCell<Instant>>> =
        Lazy::new(|| Mutex::new(RefCell::new(std::time::Instant::now())));
    let start_mtx = START.lock().unwrap();
    let start = start_mtx.borrow();

    let seconds = start.elapsed().as_secs_f64();
    let millis = seconds * 1000.0;
    let micros = (millis * 1000.0) as u64;

    info!(
        "micros = {}, millis = {}, seconds = {:.6}[sec]",
        micros, millis, seconds
    );

    let simple = Simple {
        micros,
        millis,
        seconds: format!("{:.6}[sec]", seconds),
    };
    let simple_arr = Arr {
        micros,
        millis: millis as u32,
        seconds: seconds as u8,
    };
    let simple_map = Map {
        micros,
        millis: millis as u32,
        seconds: seconds as u8,
    };
    let nested = Nested {
        ka: (micros, millis, format!("{:.6}[sec]", seconds)),
    };

    (simple, simple_arr, simple_map, nested)
}
