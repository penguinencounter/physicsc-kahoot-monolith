# Copyright 2025, PenguinEncounter
# SPDX-License-Identifier: GPL-3.0-or-later

import asyncio
import logging
import struct
import threading
import time

import serial
import serial.tools.list_ports as list_ports
from kahoot import KahootClient
from kahoot.packets.impl.respond import RespondPacket
from kahoot.packets.server import ServerPacket
from rich import print as rp

from kahoot_hooks import StatusChangedPacket, GameBeginsPacket, GetReadyPacket, QuestionBeginsPacket, QuestionEndsPacket

GAME_PIN = 8727046
NICKNAME = "ArduinoBot"

BAUD = 115200

SYM_TX = 2

die = False
ports = list(list_ports.comports())
mainSerial = serial.Serial()

client: KahootClient | None = None


class InboundPackets:
    Ping = 1
    StateChanged = 3
    Answer = 4


class OutboundPackets:
    Pong = 1
    JoinedGame = 3
    QuestionBegins = 4
    SetQuestionCount = 5
    GetReady = 6
    QuestionEnds = 7


class RemoteState:
    SYNCHRONIZE = 0
    NOT_IN_GAME = 1
    IN_LOBBY = 2
    QUESTION_INTRO = 3
    QUESTION = 4
    WAITING = 5
    QUESTION_OVER = 6


class InternalComType:
    KAHOOT_JOIN = 0
    KAHOOT_QUESTION_COUNT = 1
    KAHOOT_DC = 2
    KAHOOT_GET_READY = 3
    KAHOOT_Q_START = 4
    KAHOOT_Q_END = 5


comms = []


async def async_nop():
    pass


async def serial_thread(port: serial.Serial):
    global comms
    inbound_buffer = b''
    buf_max_size = len(b'helo') * 8
    helo_match = b'helo' * 6

    remote_state_afaik = RemoteState.SYNCHRONIZE
    actual_state = RemoteState.NOT_IN_GAME

    question_count: int | None = None

    last_username: str | None = None
    last_question: int = 0

    resend_timer = time.time()
    refresh_timer = time.time()

    def respond(packet_type: int, payload: bytes):
        full_payload = b'\x02' + packet_type.to_bytes(1) + payload
        rp(f"[blue][bold]out[/] {full_payload.hex(' ')}[/]")
        port.write(full_payload)

    def respond_game_joined(username: str):
        respond(OutboundPackets.JoinedGame, struct.pack('<16s', username.encode('utf-8')))

    def respond_question_count(count: int):
        respond(OutboundPackets.SetQuestionCount, struct.pack('<H', count))

    async def handle_sync():
        rp(f"[bright_blue]Synchronizing[/]")
        rp(f"[bright_black]from remote: [/]", end='', flush=True)
        # Dump everything else out
        while port.in_waiting:
            port.read()
        # OK the connection...
        port.write(b'HELO')

        last_att = time.time()

        # wait for acknowledgement
        while 1:
            if port.in_waiting >= 2:
                first = port.read()
                rp(f"[bright_black]{first.decode('utf-8')}[/]", end='', flush=True)
                if first == b'!':
                    break
            if time.time() - last_att > 1:
                last_att = time.time()
                if port.out_waiting < 8:
                    port.write(b'HELO')
                rp(f"[blue]?[/]", end='', flush=True)
            await asyncio.sleep(0)
        rp("\n[bright_green bold]Connected.[/]")

    async def handle_ping(_: bytes):
        rp("\n[bright_green bold]pong[/]")
        respond(OutboundPackets.Pong, b'')

    async def handle_mode_sw(payload: bytes):
        nonlocal remote_state_afaik
        remote_state_afaik = payload[0]
        rp(f"\n[bright_green bold]remote state is now {remote_state_afaik}[/]")

    async def answer(payload: bytes):
        nonlocal remote_state_afaik
        # if remote_state_afaik != RemoteState.QUESTION:
        #     return
        rp(f"\n[bright_green bold]answer {payload[0]} question {last_question}[/]")
        # love this api great
        # noinspection PyTypeChecker
        await client.send_packet(RespondPacket(str(GAME_PIN), payload[0], last_question))

    arduino_handlers = {
        InboundPackets.Ping: handle_ping,
        InboundPackets.StateChanged: handle_mode_sw,
        InboundPackets.Answer: answer
    }
    arduino_expected_length = {
        InboundPackets.Ping: 0,
        InboundPackets.StateChanged: 1,
        InboundPackets.Answer: 1,
    }

    def handle_kahoot_join(datagram: dict):
        nonlocal actual_state, remote_state_afaik, last_username
        rp(f"[bright_magenta](s) join detected[/]")
        actual_state = RemoteState.IN_LOBBY
        last_username = datagram['username']
        if remote_state_afaik != RemoteState.SYNCHRONIZE:
            rp(f"[bright_magenta](s) join sent[/]")
            respond_game_joined(datagram['username'])

    def handle_kahoot_dc(_: dict):
        rp(f"[bright_magenta](s) kahoot disconnected[/]")

    def handle_kahoot_set_q_count(datagram: dict):
        nonlocal question_count
        if question_count != datagram['count']:
            rp(f"[bright_magenta](s) Question count: {datagram['count']}[/]")
            question_count = datagram['count']
            respond_question_count(datagram['count'])

    def kahoot_send_get_ready(datagram: dict):
        nonlocal last_question
        last_question = datagram['packet'].index
        respond(OutboundPackets.GetReady, datagram['packet'].pack())

    def kahoot_send_q_start(datagram: dict):
        nonlocal last_question
        last_question = datagram['packet'].index
        respond(OutboundPackets.QuestionBegins, datagram['packet'].pack())

    def kahoot_send_q_end(datagram: dict):
        respond(OutboundPackets.QuestionEnds, datagram['packet'].pack())

    kahoot_handlers = {
        InternalComType.KAHOOT_JOIN: handle_kahoot_join,
        InternalComType.KAHOOT_QUESTION_COUNT: handle_kahoot_set_q_count,
        InternalComType.KAHOOT_DC: handle_kahoot_dc,
        InternalComType.KAHOOT_GET_READY: kahoot_send_get_ready,
        InternalComType.KAHOOT_Q_START: kahoot_send_q_start,
        InternalComType.KAHOOT_Q_END: kahoot_send_q_end
    }

    async def common_refresh():
        nonlocal refresh_timer, question_count
        if time.time() - refresh_timer > .8:
            refresh_timer = time.time()
            if question_count is not None:
                respond_question_count(question_count)

    async def not_in_game():
        nonlocal actual_state, remote_state_afaik, resend_timer
        await common_refresh()
        if actual_state == RemoteState.IN_LOBBY and time.time() - resend_timer > 0.25:
            rp(f"[bright_magenta](s) catching up: in_lobby[/]")
            resend_timer = time.time()
            handle_kahoot_join({
                'kind': InternalComType.KAHOOT_JOIN,
                'username': last_username
            })

    async def in_lobby():
        await common_refresh()

    remote_state_handlers = {
        RemoteState.SYNCHRONIZE: async_nop,
        RemoteState.NOT_IN_GAME: not_in_game,
        RemoteState.IN_LOBBY: in_lobby,
        RemoteState.QUESTION_INTRO: common_refresh,
        RemoteState.QUESTION: common_refresh,
        RemoteState.WAITING: common_refresh,
        RemoteState.QUESTION_OVER: common_refresh,
    }

    rp(f'[bright_blue]baud rate = {BAUD}[/]')
    port.baudrate = BAUD
    rp('[red]attaching...[/]')
    port.open()
    with open('netlog.txt', 'wb') as f:
        while 1:
            try:
                waiting = port.in_waiting
                if waiting:
                    if len(inbound_buffer) < buf_max_size:
                        segment = port.read(min(waiting, buf_max_size - len(inbound_buffer)))
                        f.write(segment)
                        inbound_buffer += segment
                    else:
                        segment = port.read(waiting)
                        f.write(segment)
                        inbound_buffer = inbound_buffer[waiting:] + segment
                    f.flush()

                if remote_state_afaik == RemoteState.SYNCHRONIZE:
                    if helo_match in inbound_buffer:
                        await handle_sync()
                        inbound_buffer = b''
                        remote_state_afaik = RemoteState.NOT_IN_GAME
                    else:
                        rp('[yellow]Waiting for connection.[/]')
                        await asyncio.sleep(0.25)
                else:
                    # scan for next TX code to respond to
                    if SYM_TX in inbound_buffer:
                        inbound_buffer = inbound_buffer[inbound_buffer.index(SYM_TX):]
                        if len(inbound_buffer) >= 2:
                            type_of_recv = inbound_buffer[1]
                            inbound_buffer = inbound_buffer[2:]
                            payload_len = arduino_expected_length[type_of_recv]
                            if len(inbound_buffer) < payload_len:
                                extra = arduino_expected_length[type_of_recv] - len(inbound_buffer)
                                segment = port.read(extra)
                                f.write(segment)
                                f.flush()
                                inbound_buffer = inbound_buffer + segment
                            await arduino_handlers[type_of_recv](inbound_buffer)
                            inbound_buffer = inbound_buffer[payload_len:]
                    else:
                        # disconnected?
                        if helo_match in inbound_buffer:
                            rp('[bold red]Connection lost[/] [red](remote)[/]')
                            remote_state_afaik = RemoteState.SYNCHRONIZE

                # try to get the remote in sync if possible
                await remote_state_handlers[remote_state_afaik]()

                # respond to Kahoot events
                while comms:
                    event = comms.pop()
                    rp(f'[bright_yellow]parsing internal {event}[/]')
                    kahoot_handlers[event['kind']](event)
                # safetext = inbound_buffer.decode('utf-8').replace('\n', ' ')
                # print(f"\r{len(inbound_buffer)} {safetext}".ljust(150), end='', flush=True)

                await asyncio.sleep(0)
            except KeyError:
                pass


logging.getLogger("kahootpy").setLevel(logging.DEBUG)


async def kahoot_thread():
    global comms, client
    counter = 0
    client = KahootClient()
    rp(f"[bright_magenta](k) starting kahoot client (game PIN = {GAME_PIN})[/]")
    nickname = NICKNAME

    async def rejoin(_: ServerPacket):
        nonlocal counter, nickname
        counter += 1
        nickname = f'{NICKNAME}{counter}'
        rp(f"[bright_magenta](k) retrying login with username {nickname}[/]")
        await client.join_game(GAME_PIN, nickname)

    async def status_changed(packet: StatusChangedPacket):
        rp(f"[bright_magenta](k) status changed to {packet.status}[/]")
        if packet.status == 'ACTIVE':
            comms.append({
                "kind": InternalComType.KAHOOT_JOIN,
                "username": nickname,
            })
        elif packet.status == 'MISSING':
            comms.append({
                "kind": InternalComType.KAHOOT_DC
            })

    async def game_begins(packet: GameBeginsPacket):
        rp(f"[bright_magenta](k) question count: {packet.q_count}[/]")
        comms.append({
            'kind': InternalComType.KAHOOT_QUESTION_COUNT,
            'count': packet.q_count
        })

    async def get_ready(packet: GetReadyPacket):
        rp(f"[bright_magenta](k) get ready: block {packet.index} is coming up[/]")
        comms.append({
            'kind': InternalComType.KAHOOT_QUESTION_COUNT,
            'count': packet.q_count
        })
        comms.append({
            'kind': InternalComType.KAHOOT_GET_READY,
            'packet': packet
        })

    async def question_begins(packet: QuestionBeginsPacket):
        rp(f"[bright_magenta](k) block {packet.index} is starting[/]")
        comms.append({
            'kind': InternalComType.KAHOOT_Q_START,
            'packet': packet
        })

    async def question_ends(packet: QuestionEndsPacket):
        rp(f"[bright_magenta](k) question is over.[/]")
        comms.append({
            'kind': InternalComType.KAHOOT_Q_END,
            'packet': packet
        })

    client.on('login_response_username_error', rejoin)
    client.on('kicked', rejoin)
    client.on('status_changed', status_changed)
    client.on('game_begins', game_begins)
    client.on('get_ready', get_ready)
    client.on('question_begins', question_begins)
    client.on('question_ends', question_ends)
    await client.join_game(GAME_PIN, nickname)
    print('session ended')


for p in ports:
    if 'Arduino' in (p.manufacturer or "") or 'Arduino' in (p.description or ""):
        mainSerial.port = p.device
        rp(rf'found [bold bright_green]{p.name}[/] - {p.serial_number}, [green]{p.manufacturer}[/]')
        break
else:
    rp(rf'[bold bright_red]nothing found - check your connections?[/]')
    exit(2)


async def run_all():
    await asyncio.gather(serial_thread(mainSerial), kahoot_thread())


try:
    asyncio.run(run_all())
finally:
    mainSerial.close()
