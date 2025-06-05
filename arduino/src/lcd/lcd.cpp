// Copyright 2025 PenguinEncounter
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

void setup() {
    lcd.begin(16, 2);
    lcd.print("Connect USB");
    lcd.setCursor(0, 1);
    Serial.begin(115200);
    // ReSharper disable once CppDFAConstantConditions
    while (!Serial) {}
    Serial.print("\x1brst\x06");
    for (int n = 0;;n++) {
        lcd.print(".");
        if (n % 5 == 0) {
            lcd.setCursor(0, 1);
            lcd.print("         ");
            lcd.setCursor(0, 1);
        }
        Serial.print("\x1brst\x06");
        Serial.print("\x1bping\x06");
        if (Serial.available()) {
            lcd.setCursor(0, 1);
            lcd.print("receiving...");
            // ack
            while (Serial.available() && Serial.read() != '\x06') {}
            break;
        }
        delay(250);
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ok");
}

void loop() {
    lcd.setCursor(0, 1);
    lcd.print(millis()/1000);
}