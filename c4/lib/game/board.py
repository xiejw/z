import collections
import copy

from .position import Move
from .position import Position
from .position import Color
from .snapshot import SnapshotView


# For one game use.
#
# - x should be in the range of [0, rows-1]
# - y should be in the range of [0, columns-1]
#
# - Not thread safe. Not sharable.
class Board(object):

    # - config is GameConfig
    def __init__(self, config):
        self.config = config
        self.moves = []
        self._position_dict = collections.OrderedDict()

    def new_move(self, move):
        assert _is_move_legal(self.config, self._position_dict, move), (
            'Not legal move')
        assert move.color == Color.BLACK or move.color == Color.WHITE

        self.moves.append(move)
        self._position_dict[move.position] = move.color

    def snapshot(self, deepcopy):
        if deepcopy:
            return SnapshotView(self.config, copy.deepcopy(self._position_dict))
        else:
            return SnapshotView(self.config, self._position_dict)

    # returns -1 for infeasiable.
    def next_available_row(self, column):
        for i in reversed(range(self.config.rows)):
            if Position(i, column) not in self._position_dict:
                return i
        return -1

    # returns legal positions (not moves). Could be empty.
    def legal_positions(self):
        columns = list(range(self.config.columns))
        legal_positions = []
        for c in columns:
            r = self.next_available_row(c)
            if r != -1:
                legal_positions.append(Position(r, c))
        return legal_positions

    # This assumes before last_move, there is no winner.
    #
    # Returns
    # - color of the winner if applicable.
    # - None: no winner yet
    # - Color.NA: tie
    def winner_after_last_move(self):
        last_move = self.moves[-1] if self.moves else None
        return _find_winner(self.config, self._position_dict, last_move)

    # Plots the board.
    def draw(self):
        _plot_board(self.config, self._position_dict)


# Finds winner
#
# Returns None if not find. Color.NA for tie.
def _find_winner(config, position_dict, new_move):
    if new_move == None:
        return None

    color = new_move.color
    rows = config.rows
    columns = config.columns

    if len(position_dict) == rows * columns:
        # For connect 4, if the board is full, it is a tie.
        return Color.NA

    def num_position_in_same_color(next_p):
        # Start point
        x = new_move.position.x
        y = new_move.position.y
        num = 0
        while True:
            x, y = next_p(x, y)
            if x < 0 or x >= rows:
                return num
            if y < 0 or y >= columns:
                return num

            if position_dict.get(Position(x, y)) == color:
                num += 1
                continue

            return num

    # Check horizontal.
    num_pos_on_left = num_position_in_same_color(lambda x, y: (x, y - 1))
    num_pos_on_right = num_position_in_same_color(lambda x, y: (x, y + 1))
    if num_pos_on_left + num_pos_on_right + 1 >= 4:
        return color
    del num_pos_on_left, num_pos_on_right

    # We only check down not up.
    num_pos_on_down = num_position_in_same_color(lambda x, y: (x + 1, y))
    if num_pos_on_down + 1 >= 4:
        return color
    del num_pos_on_down

    num_pos_on_left_down = num_position_in_same_color(lambda x, y: (x + 1, y - 1))
    num_pos_on_right_up = num_position_in_same_color(lambda x, y: (x - 1, y + 1))
    if num_pos_on_left_down + num_pos_on_right_up + 1 >= 4:
        return color
    del num_pos_on_left_down, num_pos_on_right_up

    num_pos_on_left_up = num_position_in_same_color(lambda x, y: (x - 1, y - 1))
    num_pos_on_right_down = num_position_in_same_color(lambda x, y: (x + 1, y + 1))
    if num_pos_on_left_up + num_pos_on_right_down + 1 >= 4:
        return color
    del num_pos_on_left_up, num_pos_on_right_down

    return None


# Checks whether the `new_move` is legal.
def _is_move_legal(config, position_dict, new_move):
    rows = config.rows
    columns = config.columns
    new_position = new_move.position

    if new_position in position_dict:
        # Should not be occupied.
        return False

    if new_position.x == rows - 1:
        # For final row, this is always OK.
        return True

    position_beneath = Position(new_position.x + 1, new_position.y)
    if position_beneath not in position_dict:
        # Now allowed. It must stack on something.
        return False

    return True


# Plots the board.
#
# Uses a free function to improve readability and isolation.
def _plot_board(config, position_dict):
    print("    ", end = '')
    for j in range(config.columns):
        print("%d " % j , end='')
    print("")

    print("    ", end = '')
    for j in range(config.columns):
        print("_ ", end='')
    print("")

    for i in range(config.rows):
        print("%2d: " % i, end='')
        for j in range(config.columns):
            color = position_dict.get(Position(i, j))
            if color is None:
                print("  ", end='')
            elif color == Color.WHITE:
                print("o ", end='')
            else:
                assert color == Color.BLACK
                print("x ", end='')

        print("")

    print("    ", end = '')
    for j in range(config.columns):
        print("_ ", end='')
    print("")

