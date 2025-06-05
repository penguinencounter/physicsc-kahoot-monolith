# Copyright 2025, PenguinEncounter
# SPDX-License-Identifier: GPL-3.0-or-later

import struct

from kahoot.packets.server import ServerPacket
from kahoot.packets.server.game_over import GameOverPacket
from kahoot.packets.server.game_start import GameStartPacket
from kahoot.packets.server.question_end import QuestionEndPacket
from kahoot.packets.server.question_ready import QuestionReadyPacket
from kahoot.packets.server.question_start import QuestionStartPacket

types = {
    'quiz': 0,
    'default': 255,
}


# on: login_response_username_error
class LoginResponseUsernameErrorPacket(ServerPacket):
    def __init__(self, data: dict):
        super().__init__(data)
        assert data['type'] == 'loginResponse'
        assert data['error'] == 'USER_INPUT'
        self.descr = data['description']


# on: status_changed
class StatusChangedPacket(ServerPacket):
    def __init__(self, data: dict):
        super().__init__(data)
        assert data['type'] == 'status'
        self.status = data['status']


# on: kicked
class KickedPacket(ServerPacket):
    def __init__(self, data: dict):
        super().__init__(data)
        assert data['id'] == 10


# on: game_begins
class GameBeginsPacket(ServerPacket):
    def __init__(self, data: dict):
        super().__init__(data)
        assert data['id'] == 9
        self.q_count = self.content['gameBlockCount']


# on: get_ready
class GetReadyPacket(ServerPacket):
    def __init__(self, data: dict):
        super().__init__(data)
        assert data['id'] == 1
        self.index = self.content['gameBlockIndex']
        self.q_count = self.content['totalGameBlockCount']
        self.type = self.content['type']
        self.duration = self.content['timeAvailable']
        self.select_count = self.content['numberOfAnswersAllowed']
        self.options = self.content['numberOfChoices']
        self.get_ready_time = self.content['getReadyTimeRemaining']

    def pack(self) -> bytes:
        return struct.pack(
            "<HBIBBI",
            self.index, types.get(self.type, types['default']),
            self.duration, self.select_count, self.options, self.get_ready_time
        )


# on: question_begins
class QuestionBeginsPacket(ServerPacket):
    def __init__(self, data: dict):
        super().__init__(data)
        assert data['id'] == 2
        self.index = self.content['gameBlockIndex']
        self.type = self.content['type']
        self.durationLeft = self.content['timeRemaining']
        self.duration = self.content['timeAvailable']
        self.select_count = self.content['numberOfAnswersAllowed']
        self.options = self.content['numberOfChoices']

    def pack(self) -> bytes:
        return struct.pack(
            "<HBIIBB",
            self.index, types.get(self.type, types['default']),
            self.duration, self.durationLeft, self.select_count, self.options
        )


# on: question_ends
class QuestionEndsPacket(ServerPacket):
    def __init__(self, data: dict):
        super().__init__(data)
        assert data['id'] == 8
        self.was_correct = self.content['isCorrect']
        self.score = self.content['totalScore']

    def pack(self) -> bytes:
        return struct.pack(
            "<I?",
            self.score, self.was_correct
        )


# Monkey-patch the provided class so that it can't be constructed
def invalidate_packet(cls):
    __class__ = cls

    # noinspection PyUnusedLocal
    def new__init__(self, data: dict):
        super().__init__(data)
        raise TypeError("no lmao")

    cls.__init__ = new__init__


# Delete all the builtin packets which are overall not good quality
invalidate_packet(QuestionStartPacket)
invalidate_packet(QuestionEndPacket)
invalidate_packet(QuestionReadyPacket)
invalidate_packet(GameOverPacket)
invalidate_packet(GameStartPacket)
