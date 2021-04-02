#pragma once
#ifndef HT_SERIAL_MSGPACK_H
#define HT_SERIAL_MSGPACK_H

#include "MsgPack/util/DebugLog/DebugLog.h"
#ifdef MSGPACK_DEBUGLOG_ENABLE
#include "MsgPack/util/DebugLog/DebugLogEnable.h"
#else
#include "MsgPack/util/DebugLog/DebugLogDisable.h"
#endif

#include "MsgPack/Types.h"
#include "MsgPack/Packer.h"
#include "MsgPack/Unpacker.h"

namespace MsgPack = ht::serial::msgpack;

#include "MsgPack/util/DebugLog/DebugLogRestoreState.h"

#endif  // ARDUINOMSGPACK_H
