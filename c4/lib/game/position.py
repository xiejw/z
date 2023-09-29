import enum
import re


# Representation of the move. See `Move`.parsing.
_move_re = re.compile(r"^([bw])@?\(\s*(\d+),\s*(\d+)\)$")

# Represents a position in game. Hashable.
class Position(object):

    def __init__(self, x, y):
        self.x = x
        self.y = y

    @staticmethod
    def of(p):
        if isinstance(p, tuple):
            x, y = p
            return Position(x,y)
        return p


    def __hash__(self):
        return hash((self.x, self.y))

    def __eq__(self, o):
        return self.x == o.x and self.y == o.y

    def __str__(self):
        return "(%d,%d)" % (self.x, self.y)


class Color(enum.Enum):
    NA = 'n/a'
    BLACK = 'b'
    WHITE = 'w'

    @staticmethod
    def of(c):
        if isinstance(c, str):
            c = Color(c)
        return c

    def reverse(self):
        assert self == Color.BLACK or self == Color.WHITE
        return Color.WHITE if self == Color.BLACK else Color.BLACK

    def __hash__(self):
        return hash(self.value)

    def __str__(self):
        return self.value

    def __eq__(self, o):
        if o is None:
            return False

        return self.value == o.value


# Represents a move in game. Basically, a position, with color
class Move(object):

    # - position is (x, y) or a Position instance
    # - color can be 'b', 'w', color.{BLACK, WHITE}
    def __init__(self, position, color):
        self.position = Position.of(position)
        self.color = Color.of(color)
        assert self.color != Color.NA

    def __str__(self):
        return "%s%s" % (self.color, self.position)

    def __hash__(self):
        return hash((self.position, self.color))

    def __eq__(self, o):
        if o is None:
            return False
        return self.position == o.position and self.color == o.color

    # reserve the `__str__`.
    #
    # The following formats are allowed.
    #
    # - b@(1,2)
    # - w(  1, 2)
    # - b@(  1, 2)
    @staticmethod
    def parsing(move_str):
        m = _move_re.search(move_str)
        if not m:
            raise RuntimeError("Invalid `Move` string representation. Got: %s"
                    % move_str)
        color = m[1]
        position = (int(m[2]), int(m[3]))
        return Move(position, color)


