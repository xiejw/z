from game import Color
from game import Position

from .policy import Policy


# Ask for next position from human input.
class HumanPolicy(Policy):

    def __init__(self, board, color, name=None):
        self._board = board
        self._color = Color.of(color)
        self.name = name if name else "human_" + color

    def next_position(self):
        b = self._board

        # Loop forever until valid input.
        while True:
            try:
                print("Column : ", end="")
                column = int(input())
                row = b.next_available_row(column)
                if row == -1:
                    print("This column is full. Try again.")
                    continue

                return Position(row, column)
            except KeyboardInterrupt as e:
                print("\033[31mAborted due to Ctrl-C.\033[0m")
                raise e
            except:
                print("Unexpected error due to invalid input. Try again.")

