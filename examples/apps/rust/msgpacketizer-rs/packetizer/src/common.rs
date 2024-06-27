use embedded_crc_macros::crc8;

/// CRC8 SMBus Plynomial (7 corresponds to the polynomial x^8 + x^2 + x + 1)
const CRC8_SMBUS_POLYNOMIAL: u8 = 7;

// Define the function to calcurate CRC8 (SMBus))
crc8!(fn crc8_smbus, CRC8_SMBUS_POLYNOMIAL, 0, "SMBus Packet Error Code");

pub struct Message {
    pub index: u8,
    pub data: Vec<u8>,
}

pub struct MessageRef<'a> {
    pub index: u8,
    pub data: &'a [u8],
}

pub fn to_hex_str(data: &[u8]) -> String {
    let mut hex = String::new();
    if log::log_enabled!(log::Level::Debug) {
        for byte in data {
            hex.push_str(&format!("0x{:02x} ", byte));
        }
    }
    hex
}
