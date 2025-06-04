#include "packet_types.h"
#include <Arduino.h>

void get_ready(question &q, const uint8_t *buffer) {
    q.index = static_cast<uint16_t>(buffer[0]) | static_cast<uint16_t>(buffer[1]) << 8;
    q.type = buffer[2];
    q.duration = static_cast<uint32_t>(buffer[3])
        | static_cast<uint32_t>(buffer[4]) << 8
        | static_cast<uint32_t>(buffer[5]) << 16
        | static_cast<uint32_t>(buffer[6]) << 24;
    q.durationLeft = q.duration;
    q.selectCount = buffer[7];
    q.optionCount = buffer[8];
    q.getReadyTime = static_cast<uint32_t>(buffer[9])
        | static_cast<uint32_t>(buffer[10]) << 8
        | static_cast<uint32_t>(buffer[11]) << 16
        | static_cast<uint32_t>(buffer[12]) << 24;
    q.getReadyBasis = millis();
}

void question_start(question &q, const uint8_t *buffer) {
    q.index = static_cast<uint16_t>(buffer[0]) | static_cast<uint16_t>(buffer[1]) << 8;
    q.type = buffer[2];
    q.duration = static_cast<uint32_t>(buffer[3])
        | static_cast<uint32_t>(buffer[4]) << 8
        | static_cast<uint32_t>(buffer[5]) << 16
        | static_cast<uint32_t>(buffer[6]) << 24;
    q.durationLeft = static_cast<uint32_t>(buffer[7])
        | static_cast<uint32_t>(buffer[8]) << 8
        | static_cast<uint32_t>(buffer[9]) << 16
        | static_cast<uint32_t>(buffer[10]) << 24;
    q.durationBasis = millis();
    q.selectCount = buffer[11];
    q.optionCount = buffer[12];
}
