// Copyright 2025 PenguinEncounter
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef OPTIONS_H
#define OPTIONS_H

#define BAUD 115200
#define MAX_PACKET_SIZE 32

#define pingTimeoutAfter 500 // ms
#define pingEvery 5000 // ms

#define SYM_TX 0x02 // STX
#define OUT_PING 0x01
#define OUT_STATE 0x03
#define OUT_ANSWER 0x04

#endif //OPTIONS_H
