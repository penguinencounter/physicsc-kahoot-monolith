import asyncio
from rich import print as rp
from kahoot import KahootClient
from kahoot.packets.server.game_start import GameStartPacket
import logging

logging.getLogger("kahootpy").setLevel(logging.DEBUG)

base_url = "https://kahoot.it"
session_reserve_url = f"{base_url}/reserve/session/%s/?%d"


async def on_joined(packet: GameStartPacket):
    rp(packet)


async def join():
    room = 6758436
    nick = "PyBot3"
    client = KahootClient()
    client.on("game_start", on_joined)
    await client.join_game(room, nick)


asyncio.run(join())
