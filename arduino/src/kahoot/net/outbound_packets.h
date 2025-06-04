#ifndef OUTBOUND_PACKETS_H
#define OUTBOUND_PACKETS_H
#include <Arduino.h>
#include <stdint.h> // NOLINT(*-deprecated-headers)
#include "buffer.h"

enum class OutboundPacketId : uint8_t {
    Reset = 0,
    Ping = 1,
    LogMessage = 2,
};

class OutboundPacket {
public:
    OutboundPacketId type;

    explicit OutboundPacket(const OutboundPacketId packetType): type(packetType) {}
    virtual ~OutboundPacket() = default;
    virtual tagged_buffer<uint8_t> generate() const {
        return {
            1,
            new uint8_t[1]{ static_cast<uint8_t>(type) }
        };
    };
};

class ResetPacket final : public OutboundPacket {
public:
    ResetPacket(): OutboundPacket(OutboundPacketId::Reset) {}
};

class PingPacket final : public OutboundPacket {
public:
    PingPacket(): OutboundPacket(OutboundPacketId::Ping) {}
};

class MessagePacket final : public OutboundPacket {
public:
    const char* reason;
    explicit MessagePacket(const char* txt): OutboundPacket(OutboundPacketId::LogMessage), reason(txt) {}
    tagged_buffer<uint8_t> generate() const override;
    ~MessagePacket() override {
        digitalWrite(13, HIGH);
        delay(100);
        digitalWrite(13, LOW);
    }
};

void send(const OutboundPacket& p);

// sends little endian
template<typename T> void send_number(T value) {
    for (size_t i = 0; i < sizeof(T); ++i) {
        Serial.write(value & 0xff);
        value >>= 8;
    }
}

#endif //OUTBOUND_PACKETS_H
