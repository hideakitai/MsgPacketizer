"use strict";
const maxApi = require('max-api');
const cobs = require('cobs');
const crc = require('crc');
const msgpack = require('msgpack-lite');

let b_index = true;
let b_crc = true;
let packet = [];

function decode(v) {
    if (v === 0) {
        if (packet.length > 2) {
            const buffer = cobs.decode(Buffer.from(packet));
            if (b_crc) {
                const crc_recv = buffer[buffer.length - 1];
                const crc_calc = crc.crc8(buffer.slice(b_index ? 1 : 0, buffer.length - 1));
                if (crc_recv === crc_calc) {
                    const decoded = msgpack.decode(buffer.slice(b_index ? 1 : 0, buffer.length - 1));
                    if (b_index) {
                        if (Array.isArray(decoded)) {
                            decoded.unshift(buffer[0]);
                            maxApi.outlet(decoded);
                        } else {
                            maxApi.outlet(buffer[0], decoded);
                        }
                    } else {
                        maxApi.outlet(decoded);
                    }
                } else {
                    maxApi.post("[COBS] crc not matched: recv = " + crc_recv + ", calc = " + crc_calc, "error");
                }
            } else {
                const decoded = msgpack.decode(buffer.slice(b_index ? 1 : 0, buffer.length));
                if (b_index) {
                    if (Array.isArray(decoded)) {
                        decoded.unshift(buffer[0]);
                        maxApi.outlet(decoded);
                    } else {
                        maxApi.outlet(buffer[0], decoded);
                    }
                } else {
                    maxApi.outlet(decoded);
                }
            }
        }
        packet = [];
    } else {
        packet.push(v);
    }
}


const handlers = {
    "verifying": (b) => {
        b_crc = b;
    },
    "indexing": (b) => {
        b_index = b;
    },
    "index": () => {
    },
    [maxApi.MESSAGE_TYPES.LIST]: (...args) => {
        args.forEach((v) => {
            decode(v);
        });
    },
    [maxApi.MESSAGE_TYPES.NUMBER]: (arg) => {
        decode(arg);
    }
};

maxApi.addHandlers(handlers);
