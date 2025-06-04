from typing import Callable

import serial
import struct
from rich import print as rp

import outbound_packets as outward
from sym import SYM_BEGIN, SYM_END


class InPacketTypes:
    Reset = 0
    Ping = 1
    LogMessage = 2


class InboundPacket:
    def __init__(self, packet_type: int):
        self.packet_type = packet_type

    def act(self, port: serial.Serial):
        raise TypeError("act() is abstract on InboundPacket")


class RemoteResetPacket(InboundPacket):
    def __init__(self):
        super().__init__(InPacketTypes.Reset)

    @classmethod
    def decode(cls, _: bytes):
        return cls()

    def act(self, port: serial.Serial):
        pass


class RemotePingPacket(InboundPacket):
    def __init__(self):
        super().__init__(InPacketTypes.Ping)

    @classmethod
    def decode(cls, _: bytes):
        return cls()

    def act(self, port: serial.Serial):
        outward.send(port, outward.LocalPongPacket())


class RemoteMessagePacket(InboundPacket):
    def __init__(self, message: str):
        super().__init__(InPacketTypes.LogMessage)
        self.message = message

    @classmethod
    def decode(cls, buf: bytes):
        return cls(buf.decode('utf-8'))

    def act(self, port: serial.Serial):
        rp(f'[magenta][bold]msg[/] {self.message}[/]')


decoders: dict[int, Callable[[bytes], InboundPacket | None]] = {
    InPacketTypes.Reset: RemoteResetPacket.decode,
    InPacketTypes.Ping: RemotePingPacket.decode,
    InPacketTypes.LogMessage: RemoteMessagePacket.decode,
}


def decode(buf: bytes) -> InboundPacket | None:
    if len(buf) < 1:
        return None
    typecode = buf[0]
    if typecode not in decoders:
        rp(f"[yellow]unknown packet with ID {typecode}[/]")
        return None
    return decoders[typecode](buf[1:])


def read(port: serial.Serial) -> InboundPacket | None:
    while True:
        if not port.in_waiting:
            return None
        bit = port.read()
        if bit == SYM_BEGIN:
            break
        elif bit == SYM_END:
            rp("[red]unclosed tag[/]")
            return None
    size = struct.unpack("<H", port.read(2))[0]
    rp(f"[bright_black]recv {size} bytes...[/]")
    buffer = port.read(size)
    rp(f'[bold green]in  {len(buffer): <3d}[/] bytes: [green]{buffer.hex(" ")}[/]')
    if port.read() != SYM_END:
        rp("[red]no end symbol, assuming corrupt[/]")
        return None
    packet = decode(buffer)
    return packet
