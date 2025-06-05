// Copyright 2025 PenguinEncounter
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>

constexpr int data = 4;
constexpr int latch = 5;
constexpr int clock = 6;
constexpr int onboardLed = 13;

uint8_t leds = 0;

void setup() {
    pinMode(data, OUTPUT);
    pinMode(latch, OUTPUT);
    pinMode(clock, OUTPUT);
    pinMode(onboardLed, OUTPUT);
    digitalWrite(onboardLed, HIGH);
    Serial.begin(9600);
    while (!Serial) {}
    digitalWrite(onboardLed, LOW);
    Serial.println("connected!");
}

void push() {
    digitalWrite(latch, LOW);
    shiftOut(data, clock, LSBFIRST, leds);
    digitalWrite(latch, HIGH);
}

void loop() {
    if (Serial.available()) {
        char c = static_cast<char>(Serial.read());
        if ('0' <= c && c <= '7') {
            const auto idx = static_cast<uint8_t>(c - '0');
            leds ^= 1 << idx;
            push();
            Serial.print("Toggled LED ");
            Serial.println(idx);
        }
    }
}
