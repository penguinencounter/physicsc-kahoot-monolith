#include "mem.h"

int availRam() {
    extern int __heap_start, *__brkval; // NOLINT(*-reserved-identifier)
    int v;
    return reinterpret_cast<int>(&v) - (__brkval == nullptr ? reinterpret_cast<int>(&__heap_start) : reinterpret_cast<int>(__brkval));
}
