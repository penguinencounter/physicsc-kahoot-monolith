#include <Arduino.h>

#include "LiquidCrystal.h"
#include "net/inbound_packets.h"
#include "net/outbound_packets.h"
#include "net/symbols.h"

#ifdef env_uno
#define useLCD
#endif

#ifdef useLCD
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
#endif
constexpr int led = 13;


int availRam() {
    extern int __heap_start, *__brkval; // NOLINT(*-reserved-identifier)
    int v;
    return reinterpret_cast<int>(&v) - (__brkval == nullptr
                                            ? reinterpret_cast<int>(&__heap_start)
                                            : reinterpret_cast<int>(__brkval));
}

void begin_comms() {
#ifdef useLCD
    lcd.begin(16, 2);
    lcd.print("Connect USB");
    lcd.setCursor(0, 1);
#endif
    Serial.begin(115200);
    while (!Serial) {
    }
    {
        send(MessagePacket("RST"));
    }
    // digitalWrite(led, HIGH);
    delay(500);
    // digitalWrite(led, LOW);

    Serial.print(F("[- mem = "));
    Serial.print(availRam());
    Serial.print(F(" -]"));
    return;

    const InboundPacket *recv = nullptr;
    for (int n = 0;; n++) {
        send(ResetPacket());
        send(PingPacket());
        recv = read();
        if (recv != nullptr) {
            if (recv->type == InboundPacketId::Pong) {
                delete recv;
                break;
            }
            if (recv->type == InboundPacketId::Error) {
                if (recv == &PacketReadErrors::INVALID) send(MessagePacket("r1"));
                else if (recv == &PacketReadErrors::UNCLOSED_TAG) send(MessagePacket("r2"));
                else if (recv == &PacketReadErrors::NO_SYM_END) send(MessagePacket("r3"));
                else send(MessagePacket("r4"));
            }
        }
        delete recv;
        delay(250);
    }

    send(MessagePacket("Connected!"));
}

void setup() {
    pinMode(led, OUTPUT);
    begin_comms();
}

void loop() {
    delay(500);
}
