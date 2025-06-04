import struct
from rich import print as rp

import serial
from sym import *


class OutPacketTypes:
    Reset = 0
    Pong = 1


class OutboundPacket:
    def __init__(self, packet_type: int):
        self.packet_type = packet_type

    def generate(self) -> bytes:
        return bytes([self.packet_type])


class LocalResetPacket(OutboundPacket):
    def __init__(self):
        super().__init__(OutPacketTypes.Reset)


class LocalPongPacket(OutboundPacket):
    def __init__(self):
        super().__init__(OutPacketTypes.Pong)

    def generate(self) -> bytes:
        return bytes([self.packet_type, 0x31, 0x41, 0x59, 0x26])


def send(port: serial.Serial, packet: OutboundPacket):
    msg = packet.generate()
    sizeof = len(msg)
    full = struct.pack(f'<cH{sizeof}sc', SYM_BEGIN, sizeof, msg, SYM_END)
    rp(f'[bold blue]out {len(full): <3d}[/] bytes: [blue]{full.hex(" ")}[/]')
    port.write(full)
