#include "outbound_packets.h"

#include "symbols.h"

tagged_buffer<uint8_t> MessagePacket::generate() const {
    const size_t len = strlen(this->reason) + 1;
    const size_t size = 1 + len;
    auto* buffer = new uint8_t[size] {static_cast<uint8_t>(type)};
    memcpy(buffer+1, this->reason, len);
    return {
        size,
        buffer
    };
}

void send(const OutboundPacket& p) {
    const tagged_buffer<uint8_t> buf = p.generate();
    Serial.write(SYM_BEGIN);
    send_number(buf.size);
    Serial.write(buf.ptr, buf.size);
    Serial.write(SYM_END);
}
