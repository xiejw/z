from .board import Board

# Should be treated as immutable
class GameConfig(object):

    def __init__(self, columns=7, rows=6):
        self.columns = columns
        self.rows = rows
        assert columns < 10, 'columns should be less than 10'
        assert rows < 10, 'rows should be less than 10'

    def __str__(self):
        return "Connect 4 Game Config (%dx%d)" % (self.rows, self.columns)

    def new_board(self):
        return Board(self)

    def convert_position_to_index(self, position):
        return position.x * self.columns + position.y
