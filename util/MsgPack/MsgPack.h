#pragma once
#ifndef HT_SERIAL_MSGPACK_H
#define HT_SERIAL_MSGPACK_H

#ifndef MSGPACK_ENABLE_DEBUG_LOG
#define NDEBUG // disable conversion warning
#endif

#include "MsgPack/Types.h"
#include "MsgPack/Packer.h"
#include "MsgPack/Unpacker.h"

namespace MsgPack = ht::serial::msgpack;

#endif // ARDUINOMSGPACK_H
