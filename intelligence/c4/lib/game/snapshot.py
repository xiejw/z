import collections

from .position import Color
from .position import Move
from .position import Position


class SnapshotView(object):

    # call site make deepcopy. So this is a view.
    def __init__(self, config, position_dict):
        self._config = config
        self._position_dict = position_dict
        assert isinstance(position_dict, collections.OrderedDict)

    # returns a compact version of snapshot.
    def __str__(self):
        moves = []
        for position, color in self._position_dict.items():
            move = Move((position.x, position.y), color)
            moves.append(move.__str__())
        return '^'.join(moves)


    # reverse logic of `__str__`.
    @staticmethod
    def parsing(config, snapshot_str):
        # For empty string, fast return.
        if not snapshot_str:
            return SnapshotView(config, collections.OrderedDict())

        position_dict = collections.OrderedDict()
        for move_str in snapshot_str.split("^"):
            move = Move.parsing(move_str)
            position_dict[move.position] = move.color

        return SnapshotView(config, position_dict)

    # returns color. None means no present.
    def get(self, position):
        return self._position_dict.get(position)


    # returns a compact version of board drawing.
    def board_view(self):
        config = self._config

        s = ''

        for i in range(config.rows):
            s += ("%2d: " % i)
            for j in range(config.columns):
                color = self._position_dict.get(Position(i, j))
                if color is None:
                    s += "  "
                elif color == Color.WHITE:
                    s += "o "
                else:
                    assert color == Color.BLACK
                    s += "x "

            s += "\n"
        return s


