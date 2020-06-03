"use strict";
const maxApi = require('max-api');
const cobs = require('cobs');
const crc = require('crc');
const msgpack = require('msgpack-lite');

let b_index = true;
let b_crc = true;
let index = 0;

function encode(buf)
{
    let arr = Array.from(buf);
    if (b_crc)
        arr.push(crc.crc8(buf));
    if (b_index)
        arr.unshift(index);
    let cobs_arr = Array.from(cobs.encode(Buffer.from(arr)));
    cobs_arr.push(0)
    maxApi.outlet(cobs_arr);
}

const handlers = {
    "verifying": (b) => {
        b_crc = b;
    },
    "indexing": (b) => {
        b_index = b;
    },
    "index": (i) => {
        index = i % 256;
    },
    [maxApi.MESSAGE_TYPES.LIST]: (...args) => {
        const buf = msgpack.encode(args);
        encode(buf);
    },
    [maxApi.MESSAGE_TYPES.DICT]: (args) => {
        const buf = msgpack.encode(args);
        encode(buf);
    }
};

maxApi.addHandlers(handlers);
