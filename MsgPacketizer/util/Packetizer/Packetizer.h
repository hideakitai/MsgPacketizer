#pragma once
#ifndef HT_SERIAL_PACKETIZER
#define HT_SERIAL_PACKETIZER

#ifdef ARDUINO
#include <Arduino.h>
#endif

#if defined(ARDUINO) || defined(OF_VERSION_MAJOR) || defined(SERIAL_H)
#define PACKETIZER_ENABLE_STREAM
#ifdef ARDUINO  // TODO: support more platforms
#if defined(ESP_PLATFORM) || defined(ESP8266) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_SAMD_MKRVIDOR4000) || defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_SAMD_NANO_33_IOT)
#define PACKETIZER_ENABLE_WIFI
#endif
#if defined(ESP_PLATFORM) || defined(ESP8266) || !defined(PACKETIZER_ENABLE_WIFI)
#define PACKETIZER_ENABLE_ETHER
#endif
#endif
#endif

#if defined(PACKETIZER_ENABLE_ETHER) || defined(PACKETIZER_ENABLE_WIFI)
#define PACKETIZER_ENABLE_NETWORK
#include <Udp.h>
#include <Client.h>
#endif

#include "Packetizer/util/ArxTypeTraits/ArxTypeTraits.h"
#include "Packetizer/util/ArxContainer/ArxContainer.h"
#include "Packetizer/util/ArxSmartPtr/ArxSmartPtr.h"
#include "Packetizer/Types.h"
#include "Packetizer/Encoding.h"
#include "Packetizer/Encoder.h"
#include "Packetizer/Decoder.h"

namespace Packetizer = arduino::packetizer;

#endif  // HT_SERIAL_PACKETIZER
