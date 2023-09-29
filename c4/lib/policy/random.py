import random

from game import Color

from .policy import Policy


# Randomly selects the position.
#
# This is a very good for baseline or for bootstrap.
class RandomPolicy(Policy):

    def __init__(self, board, color, name=None):
        self._board = board
        self._color = Color.of(color)
        self.name = name if name else "random_" + color

    def next_position(self):
        b = self._board
        legal_positions = b.legal_positions()

        if not legal_positions:
            raise RuntimeError("Game is over already.")

        return random.choice(legal_positions)

