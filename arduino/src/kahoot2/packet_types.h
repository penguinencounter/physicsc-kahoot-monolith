#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef void(* packet_callback)();

enum InPacketType {
    IN_UNASSIGNED_0 = 0,
    IN_PONG = 1,
    IN_UNASSIGNED_2 = 2,
    IN_JOINED_GAME = 3,
    IN_QUESTION_BEGINS = 4,
    IN_SET_QUESTION_COUNT = 5,
    IN_GET_READY = 6,
    IN_QUESTION_ENDS = 7,
    _in_lut_size,
};

struct get_ready {
    uint16_t q_num;
    uint8_t mode;
    uint8_t number_of_answers;
};

constexpr size_t read_sizes[_in_lut_size] = {
    [IN_UNASSIGNED_0] = 0,
    [IN_PONG] = 0,
    [IN_UNASSIGNED_2] = 0,
    [IN_JOINED_GAME] = 16,
    [IN_QUESTION_BEGINS] = 13,
    [IN_SET_QUESTION_COUNT] = 2,
    [IN_GET_READY] = 13,
    [IN_QUESTION_ENDS] = 5, // score int32, correct?
};

struct question {
    uint16_t index;
    uint8_t type;
    uint32_t duration;
    uint32_t durationLeft;
    unsigned long durationBasis;
    uint8_t selectCount;
    uint8_t optionCount;
    uint32_t getReadyTime;
    unsigned long getReadyBasis;
};

void get_ready(question& q, const uint8_t* buffer);
void question_start(question& q, const uint8_t* buffer);

#endif //PACKET_TYPES_H
