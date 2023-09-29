import os

from data import InferenceState
from game import Color
from game import Move
from game import Position
from model import convert_inference_state_to_model_feature

from .policy import Policy

import xai_c4

# A policy based on parallel MCTS and trained model.
#
# - See AlphaGo Zero paper for details.
class MCTSParPolicy(Policy):


    def __init__(self, board, color, iterations=1600, name=None):
        self._board = board
        self._config = board.config
        self._color = Color.of(color)
        self._iterations = iterations
        self.name = name if name else "mcts_par_" + color

    def __del__(self):
        xai_c4.cleanup()

    def next_position(self):
        # TODO this is an optimize we can do is incremental build the board in
        # cc side.

        row_count = self._config.rows
        col_count = self._config.columns
        pos_dict = self._board._position_dict
        pos = [0] * (row_count * col_count)

        for row in range(row_count):
            off = row * col_count
            for col in range (col_count):
                color = pos_dict.get(Position.of((row, col)))
                if color is None:
                    continue

                pos[off + col] = 1 if color == Color.BLACK else -1

        pos_idx = xai_c4.select_next_move(
                pos,
                1 if self._color == Color.BLACK else -1)
        row = pos_idx // col_count
        col = pos_idx % col_count

        pos = Position.of((row, col))
        print("==> from cc", pos)

        return pos


