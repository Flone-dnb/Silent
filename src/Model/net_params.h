// This file is part of the Silent.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.

#pragma once

// Limits
#define  MAX_NAME_LENGTH                20
#define  MAX_BUFFER_SIZE                1420
#define  MAX_VERSION_STRING_LENGTH      20
#define  MAX_TCP_BUFFER_SIZE            9000 // note: also change in the server
#define  MAX_MESSAGE_LENGTH             550  // note: actual size is "MAX_MESSAGE_LENGTH * 2" because we use std::wstring.
                                             // please, don't set this value more than 680 (1400 (~MTU) / 2 - MAX_NAME_LENGTH).


// TCP / UDP
#define  INTERVAL_TCP_MESSAGE_MS        120
#define  INTERVAL_UDP_MESSAGE_MS        2
#define  INTERVAL_KEEPALIVE_SEC         20   // note: also change in server
#define  CHECK_IF_SERVER_DIED_EVERY_MS  800  // note: also used in disconnect() and answerToFIN(): "Wait for serverMonitor() to end".
#define  INTERVAL_AUDIO_RECORD_MS       15
#define  ATTEMPTS_TO_DISCONNECT_COUNT   5

// Ping
#define  PING_CHECK_INTERVAL_SEC        50
