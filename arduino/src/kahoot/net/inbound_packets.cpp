#include "inbound_packets.h"
#include "symbols.h"

void PacketReadError::act() {}

const InboundPacket *InboundResetPacket::decode(tagged_buffer<uint8_t> *) {
    /* no data */
    return new InboundResetPacket();
}

void InboundResetPacket::act() {}

const InboundPacket *PongPacket::decode(tagged_buffer<uint8_t> * buf) {
    // pi reference :P
    if (buf->next() != 0x31) return &PacketReadErrors::INVALID;
    if (buf->next() != 0x41) return &PacketReadErrors::INVALID;
    if (buf->next() != 0x59) return &PacketReadErrors::INVALID;
    if (buf->next() != 0x26) return &PacketReadErrors::INVALID;
    return new PongPacket();
}

void PongPacket::act() {}

const InboundPacket *decode(tagged_buffer<uint8_t> *data) {
    const uint8_t id = *data->ptr;
    data->ptr++;
    data->size--;
    switch (static_cast<InboundPacketId>(id)) {
        case InboundPacketId::Error: return &PacketReadErrors::LITERALLY_AN_ERROR;
        case InboundPacketId::Reset: return InboundResetPacket::decode(data);
        case InboundPacketId::Pong: return PongPacket::decode(data);
    }
    return nullptr;
}

template <typename T> T& move(T& in) noexcept { return in; }

const InboundPacket *read() {
    for (;;) {
        if (!Serial.available()) return nullptr;
        const uint8_t sym = Serial.read();
        if (sym == SYM_BEGIN) break;
        /* ??? */
        if (sym == SYM_END) return &PacketReadErrors::UNCLOSED_TAG;
    }
    const auto size = read_number_imm<size_t>();
    auto buf = tagged_buffer<uint8_t> {
        size,
        new uint8_t[size] {}
    };
    // read it out
    for (size_t write_loc = 0; write_loc < size; write_loc++) {
        buf.ptr[write_loc] = Serial.read();
    }
    if (Serial.read() != SYM_END) {
        return &PacketReadErrors::NO_SYM_END;
    }
    const InboundPacket* result = decode(&buf);
    // assert SYM_END
    return result;
}
