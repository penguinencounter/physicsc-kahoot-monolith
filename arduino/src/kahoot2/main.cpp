// Copyright 2025 PenguinEncounter
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include "mem.h"
#include "options.h"
#include "packet_types.h"
#include "resetdbg.h"
#include "stdint.h"
#include "stddef.h"

#ifdef env_uno
#include "LiquidCrystal.h"
#define useLCD
#endif

#ifdef useLCD
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
#endif
constexpr int led = 13;
constexpr int led_dbg = 2;

constexpr int button1 = 2;
constexpr int button2 = 3;
constexpr int button3 = 4;
constexpr int button4 = 5;

typedef const __FlashStringHelper *FlashString;

constexpr unsigned long sync_delay = 250;

bool isTicking = false;
void CHECK_SIGNAL() {
    // digitalWrite(led_dbg, isTicking ? HIGH : LOW);
    isTicking = !isTicking;
}

inline void setup_lcd() {
#ifdef useLCD
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print(F("Starting..."));
#endif
}

inline void set_status(FlashString str) {
#ifdef useLCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(str);
#else
    Serial.print(F("// "));
    Serial.println(str);
#endif
}

int blink = 0;

inline void sync_com() {
    Serial.println(F("// resyncing"));
    digitalWrite(led, HIGH);

    // Ask the remote if it can hear us...
    for (;; blink = !blink) {
        CHECK_SIGNAL();
        digitalWrite(led, blink ? HIGH : LOW);
        const auto avail = Serial.available();
        if (avail < 4) {
            Serial.print(F("helo"));
            delay(sync_delay);
        }
        if (avail >= 4) {
            // try to read 'HELO'
            if (Serial.read() != 'H') continue;
            if (Serial.read() != 'E') continue;
            if (Serial.read() != 'L') continue;
            if (Serial.read() != 'O') continue;
            break;
        }
    }
    Serial.print(F("!"));
    digitalWrite(led, LOW);
}

inline void print_system_status() {
    Serial.print(F("\n// mem "));
    Serial.print(availRam());
    Serial.print(F("B\n"));
}

inline void send_answer(const uint8_t choice) {
    Serial.write('\x02');
    Serial.write(OUT_ANSWER);
    Serial.write(static_cast<char>(choice));
}

enum StateMachine: uint8_t {
    /* Idle states */
    SYNCHRONIZE,
    NOT_IN_GAME,
    IN_LOBBY,
    /* Active states */
    QUESTION_INTRO,
    QUESTION,
    WAITING,
    QUESTION_OVER
};

StateMachine state = SYNCHRONIZE;

inline void enter_synchronize();

inline void enter_not_in_game();

inline void enter_in_lobby();
inline void enter_question_intro();
inline void enter_question();
inline void enter_waiting();
inline void enter_question_over();

unsigned long lastPing = 0;
uint16_t questionCount = 0xffff;
bool pingAcked = true;

void setup() {
    Serial.begin(BAUD);
    printReset();
    Serial.println(F("// reset"));
    pinMode(led, OUTPUT);
    pinMode(button1, INPUT_PULLUP);
    pinMode(button2, INPUT_PULLUP);
    pinMode(button3, INPUT_PULLUP);
    pinMode(button4, INPUT_PULLUP);
    // pinMode(led_dbg, OUTPUT);
    setup_lcd();
}

// State machine collection 1
// (checks connection periodically)
bool keepalive() {
    const unsigned long now = millis();
    if (now - lastPing > pingEvery) {
        lastPing = now;
        pingAcked = false;
        Serial.write(SYM_TX);
        Serial.write(OUT_PING);
    }
    if (!pingAcked && now - lastPing > pingTimeoutAfter) {
        enter_synchronize();
        return true;
    }
    return false;
}

int rxPacketType = -1;
size_t expectedRemain = 0;
bool isPacketInProgress = false;
uint8_t shared_buf[MAX_PACKET_SIZE] = {};
uint8_t *writePtr = shared_buf;

question current_question = {};
uint32_t score = 0;
bool wasLastQuestionCorrect = false;

void process_pong() {
    // 0 bytes.
    pingAcked = true;
}

void process_joined_game() {
    // 16 bytes.
#ifdef useLCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Joined!"));
    lcd.setCursor(0, 1);
    lcd.print(reinterpret_cast<char*>(shared_buf));
#else
    Serial.print(F("// user: "));
    Serial.println(reinterpret_cast<char*>(shared_buf));
#endif
    enter_in_lobby();
}

void process_set_q_count() {
    questionCount = 0;
    questionCount |= shared_buf[0];
    questionCount |= static_cast<uint16_t>(shared_buf[1]) << 8;
}

void process_question_begins() {
    question_start(current_question, shared_buf);
    enter_question();
}

void process_get_ready() {
    get_ready(current_question, shared_buf);
    enter_question_intro();
}

void process_question_ends() {
    score = static_cast<uint32_t>(shared_buf[0])
        | static_cast<uint32_t>(shared_buf[1]) << 8
        | static_cast<uint32_t>(shared_buf[2]) << 16
        | static_cast<uint32_t>(shared_buf[3]) << 24;
    wasLastQuestionCorrect = static_cast<bool>(shared_buf[4]);
    enter_question_over();
}

packet_callback handlers[_in_lut_size] = {
    [IN_UNASSIGNED_0] = nullptr,
    [IN_PONG] = process_pong,
    [IN_UNASSIGNED_2] = nullptr,
    [IN_JOINED_GAME] = process_joined_game,
    [IN_QUESTION_BEGINS] = process_question_begins,
    [IN_SET_QUESTION_COUNT] = process_set_q_count,
    [IN_GET_READY] = process_get_ready,
    [IN_QUESTION_ENDS] = process_question_ends,
};

void new_packet();

void continue_packet();

void continue_packet() { // NOLINT(misc-no-recursion)
    const size_t avail = Serial.available();
    const size_t toRead = avail < expectedRemain ? avail : expectedRemain;
    const size_t actuallyRead = Serial.readBytes(writePtr, toRead);
    writePtr += actuallyRead;
    expectedRemain -= actuallyRead;
    if (expectedRemain <= 0) {
        /* Handle it. */
        Serial.print(F("proc id "));
        Serial.println(rxPacketType);
        handlers[rxPacketType]();
        writePtr = shared_buf;
        isPacketInProgress = false;
        return new_packet();
    }
}

void new_packet() { // NOLINT(misc-no-recursion)
    // Scan until a SYM_TX
    if (Serial.available() < 2) return;
    while (Serial.available() >= 2) {
        const int byte = Serial.read();
        if (byte == -1) return;
        if (byte == SYM_TX) goto ok;
    }
    return;
    ok:
    const int sym_type = Serial.read();
    Serial.print(F("reading id "));
    Serial.println(sym_type);
    if (sym_type == -1) return;
    if (sym_type >= _in_lut_size) return;
    rxPacketType = sym_type;
    expectedRemain = read_sizes[sym_type];
    isPacketInProgress = true;
    continue_packet();
}

void packet_entrypoint() {
    if (isPacketInProgress) return continue_packet();
    return new_packet();
}

inline void print_q_header() {
#ifdef useLCD
    lcd.setCursor(0, 0);
    lcd.print(current_question.index + 1);
    lcd.print(F(" of "));
    lcd.print(questionCount);
    lcd.print(F("        "));
#else
    Serial.print(F("// Question "));
    Serial.print(current_question.index + 1);
    Serial.print(F(" of "));
    Serial.println(questionCount);
#endif
}

inline void send_state_change(const uint8_t state) {
    Serial.write('\x02');
    Serial.write(OUT_STATE);
    Serial.write(static_cast<char>(state));
}

void enter_synchronize() {
    set_status(F("Conn lost..."));
    state = SYNCHRONIZE;
    // just. throw it away.
    while (Serial.available()) {
        Serial.read();
    }
    print_system_status();
}


void enter_not_in_game() {
    state = NOT_IN_GAME;
    questionCount = 0xffff; // Unsure at this point
    lastPing = millis();
    pingAcked = true;
    print_system_status();
    set_status(F("Joining..."));
    send_state_change(NOT_IN_GAME);
}

void enter_in_lobby() {
    state = IN_LOBBY;
    send_state_change(IN_LOBBY);
}

void enter_question_intro() {
    state = QUESTION_INTRO;
    send_state_change(QUESTION_INTRO);
#ifndef useLCD
    print_q_header();
#endif
}

void enter_waiting() {
    state = WAITING;
    send_state_change(WAITING);
#ifdef useLCD
    lcd.clear();
    print_q_header();
    lcd.setCursor(0, 1);
    lcd.print(F("Hold on..."));
#endif
}

void enter_question_over() {
    state = QUESTION_OVER;
    send_state_change(QUESTION_OVER);
#ifdef useLCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(current_question.index + 1);
    lcd.print(F(" of "));
    lcd.print(questionCount);
    lcd.print(F(" - "));
    lcd.print(score);
    lcd.setCursor(0, 1);
    if (wasLastQuestionCorrect) {
        lcd.print(F("Correct!"));
    } else {
        lcd.print(F("Incorrect."));
    }
#endif
}


void enter_question() {
    state = QUESTION;
    send_state_change(QUESTION);
#ifndef useLCD
    print_q_header();
#endif
}

void idling_loop() {
    digitalWrite(led, isPacketInProgress ? HIGH : LOW);
    keepalive();
    packet_entrypoint();
}

void in_lobby_text() {
#ifdef useLCD
    lcd.setCursor(0, 0);
    if (questionCount != 0xffff) {
        lcd.print(questionCount);
        lcd.print(F(" questions      "));
    } else {
        lcd.print(F("You're in!      "));
    }
#endif
}

void question_intro() {
#ifdef useLCD
    print_q_header();
    lcd.setCursor(0, 1);
    const unsigned long timeLeft = current_question.getReadyTime - (millis() - current_question.getReadyBasis);
    const int seconds = static_cast<int>(timeLeft) / 1000;
    lcd.print(F("READY? "));
    lcd.print(seconds);
    lcd.print(F("  "));
#endif
}

void in_question() {
#ifdef useLCD
    print_q_header();
    lcd.setCursor(0, 1);
    const unsigned long duration = current_question.durationLeft - (millis() - current_question.durationBasis);
    const auto display = static_cast<uint8_t>(duration * 16 / current_question.duration);
    if (display > 16) return;
    for (uint8_t i = 0; i < display; i++) lcd.print("#");
    for (uint8_t i = 0; i < 16 - display; i++) lcd.print(" ");
#endif

    if (digitalRead(button1) == LOW && current_question.optionCount >= 1) {
        send_answer(0);
        goto answered;
    }
    if (digitalRead(button2) == LOW && current_question.optionCount >= 2) {
        send_answer(1);
        goto answered;
    }
    if (digitalRead(button3) == LOW && current_question.optionCount >= 3) {
        send_answer(2);
        goto answered;
    }
    if (digitalRead(button4) == LOW && current_question.optionCount >= 4) {
        send_answer(3);
        goto answered;
    }

    return;
    answered:
    enter_waiting();
}

void loop() {
    CHECK_SIGNAL();
    switch (state) {
        case SYNCHRONIZE:
            sync_com();
            enter_not_in_game();
            break;
        case IN_LOBBY:
            in_lobby_text();
            break;
        case NOT_IN_GAME:
            break;
        case QUESTION_INTRO:
            question_intro();
            break;
        case QUESTION:
            in_question();
            break;
        case WAITING:
            break;
        case QUESTION_OVER:
            break;
    }
    if (state != SYNCHRONIZE)
        idling_loop();
    delay(50);
}
