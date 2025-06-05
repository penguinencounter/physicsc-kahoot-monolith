// Copyright 2025 PenguinEncounter
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mem.h"

// this code is pretty standardized
int availRam() {
    extern int __heap_start, *__brkval; // NOLINT(*-reserved-identifier)
    int v;
    return reinterpret_cast<int>(&v) - (__brkval == nullptr ? reinterpret_cast<int>(&__heap_start) : reinterpret_cast<int>(__brkval));
}
