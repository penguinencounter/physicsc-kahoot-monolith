#include <Arduino.h>

constexpr int LED = 13;

constexpr int dot = 80;
constexpr int dash = 3*dot;
constexpr int letter_boundary = 3*dot - dot;
constexpr int word_boundary = 7*dot - letter_boundary - dot;

void hi() { digitalWrite(LED, HIGH); }
void lo() { digitalWrite(LED, LOW); }

void send_dot() {
  hi();
  delay(dot);
  lo();
  delay(dot);
}

void send_dash() {
  hi();
  delay(dash);
  lo();
  delay(dot);
}

struct signal {
  size_t size;
  bool values[5];
};

constexpr bool _do = false;
constexpr bool _da = true;

constexpr signal letters[26] = {
  { 2, { _do, _da } }, // a
  { 4, { _da, _do, _do, _do } }, // b
  { 4, { _da, _do, _da, _do } }, // c
  { 3, { _da, _do, _do } }, // d
  { 1, { _do } }, // e
  { 4, { _do, _do, _da, _do } }, // f
  { 3, { _da, _da, _do } }, // g
  { 4, { _do, _do, _do, _do } }, // h
  { 2, { _do, _do } }, // i
  { 4, { _do, _da, _da, _da } }, // j
  { 3, { _da, _do, _da } }, // k
  { 4, { _do, _da, _do, _do } }, // l
  { 2, { _da, _da } }, // m
  { 2, { _da, _do } }, // n
  { 3, { _da, _da, _da } }, // o
  { 4, { _do, _da, _da, _do } }, // p
  { 4, { _da, _da, _do, _da } }, // q
  { 3, { _do, _da, _do } }, // r
  { 3, { _do, _do, _do } }, // s
  { 1, { _da } }, // t
  { 3, { _do, _do, _da } }, // u
  { 4, { _do, _do, _do, _da } }, // v
  { 3, { _do, _da, _da } }, // w
  { 4, { _da, _do, _do, _da } }, // x
  { 4, { _da, _do, _da, _da } }, // y
  { 4, { _da, _da, _do, _do } }, // z
};

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
}

char basis = 'a';

void send(String str) {
  for (auto chr : str) {
    Serial.print(chr);
    if (chr == ' ') {
      delay(letter_boundary + word_boundary);
      continue;
    }
    const signal& value = letters[static_cast<size_t>(chr - basis)];
    for (size_t n = 0; n < value.size; n++) {
      if (value.values[n]) send_dash();
      else send_dot();
    }
    delay(letter_boundary);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  send("hello nishk how are you doing today");
  Serial.println();
  delay(word_boundary);
}