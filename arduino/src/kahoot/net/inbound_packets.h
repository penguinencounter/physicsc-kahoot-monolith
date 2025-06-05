// Copyright 2025 PenguinEncounter
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INBOUND_PACKETS_H
#define INBOUND_PACKETS_H

#include <stdint.h>
#include <stddef.h>
#include <Arduino.h>

#include "buffer.h"

enum class InboundPacketId : uint8_t {
    Error = 0xff,
    Reset = 0,
    Pong = 1,
};

class InboundPacket {
public:
    InboundPacketId type;

    explicit InboundPacket(const InboundPacketId packetType): type(packetType) {}
    virtual ~InboundPacket() = default;
    virtual void act() = 0;
};

class PacketReadError final : public InboundPacket {
public:
    enum class Reason : uint8_t {
        UNCLOSED_TAG,
        NO_SYM_END,
        LITERALLY_AN_ERROR,
        INVALID
    };
    const Reason reason;

    void act() override;

    explicit PacketReadError(const Reason why): InboundPacket(InboundPacketId::Error), reason(why) {}
};

namespace PacketReadErrors {
    const PacketReadError UNCLOSED_TAG { PacketReadError::Reason::UNCLOSED_TAG };
    const PacketReadError NO_SYM_END { PacketReadError::Reason::NO_SYM_END };
    const PacketReadError LITERALLY_AN_ERROR { PacketReadError::Reason::LITERALLY_AN_ERROR };
    const PacketReadError INVALID { PacketReadError::Reason::INVALID };
}

class InboundResetPacket final : public InboundPacket {
public:
    InboundResetPacket(): InboundPacket(InboundPacketId::Reset) {}
    static const InboundPacket *decode(tagged_buffer<uint8_t> *);

    void act() override;
};

class PongPacket final : public InboundPacket {
public:
    PongPacket(): InboundPacket(InboundPacketId::Pong) {}
    static const InboundPacket *decode(tagged_buffer<uint8_t> *);

    void act() override;
};

const InboundPacket *decode(tagged_buffer<uint8_t> *data);

const InboundPacket *read();

template<typename T> T read_number(tagged_buffer<uint8_t>* data) {
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        result <<= 8;
        result |= data->ptr[sizeof(T) - i - 1];
    }
    data->ptr += sizeof(T);
    data->size -= sizeof(T);
    return result;
}

template<typename T> T read_number_imm() {
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        result <<= 8;
        result |= Serial.read();
    }
    return result;
}

#endif //INBOUND_PACKETS_H
