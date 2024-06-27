use bevy::log::LogPlugin;
use bevy::prelude::*;
use bevy_serial::{SerialPlugin, SerialReadEvent, SerialWriteEvent};
use clap::Parser;
use msgpacketizer::{self, Message};
use once_cell::sync::{Lazy, OnceCell};
use serde::{Deserialize, Serialize};
use std::cell::RefCell;
use std::sync::Mutex;
use std::time::Instant;

// to write data to serial port periodically
#[derive(Resource)]
struct SerialWriteTimer(Timer);

static SUBSCRIBER: OnceCell<Mutex<msgpacketizer::Subscriber<1024>>> = OnceCell::new();

const SERIAL_PORT: &str = "/dev/ttyACM0";

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

    // set to global variables lazily
    let _ = SUBSCRIBER.set(Mutex::new(subscriber));

    info!("start bevy app!!");
    println!("start bevy app!!");

    App::new()
        .add_plugins(MinimalPlugins)
        .add_plugins(LogPlugin {
            // filter: "info,wgpu_core=warn,wgpu_hal=warn,mygame=debug".into(),
            // level: bevy::log::Level::DEBUG,
            ..default()
        })
        // simply specify port name and baud rate for `SerialPlugin`
        .add_plugins(SerialPlugin::new(SERIAL_PORT, cli.baudrate))
        // to write data to serial port periodically (every 1 second)
        .insert_resource(SerialWriteTimer(Timer::from_seconds(
            0.1,
            TimerMode::Repeating,
        )))
        // reading and writing from/to serial port is achieved via bevy's event system
        .add_systems(Update, read_serial)
        .add_systems(Update, write_serial)
        .run();
}

// reading event for serial port
fn read_serial(mut ev_serial: EventReader<SerialReadEvent>) {
    let subscriber_mtx = SUBSCRIBER.get().unwrap();
    // you can get label of the port and received data buffer from `SerialReadEvent`
    for SerialReadEvent(label, buffer) in ev_serial.read() {
        let s = String::from_utf8(buffer.clone()).unwrap();
        debug!("Received packet from {label}: {s}");

        let mut subscriber = subscriber_mtx.lock().unwrap();
        let remained = subscriber.feed(buffer.as_slice());
        if remained > 0 {
            debug!("Packet remained = {remained}");
        }
    }
}

// writing event for serial port
fn write_serial(
    mut ev_serial: EventWriter<SerialWriteEvent>,
    mut timer: ResMut<SerialWriteTimer>,
    time: Res<Time>,
) {
    // write msg to serial port every 1 second not to flood serial port
    if timer.0.tick(time.delta()).just_finished() {
        let (simple, simple_arr, simple_map, custom) = generate_current_data();
        let mut packet = vec![];
        packet.extend(msgpacketizer::encode(SEND_INDEX_SIMPLE, simple).unwrap());
        packet.extend(msgpacketizer::encode(SEND_INDEX_ARR, simple_arr).unwrap());
        packet.extend(msgpacketizer::encode_named(SEND_INDEX_MAP, simple_map).unwrap());
        packet.extend(msgpacketizer::encode_named(SEND_INDEX_CUSTOM, custom).unwrap());

        // you can write to serial port via `SerialWriteEvent` with label and buffer to write
        ev_serial.send(SerialWriteEvent(SERIAL_PORT.to_string(), packet));
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
