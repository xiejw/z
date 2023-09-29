
# Detects whether a board has been seen before.
class DupDetector(object):

    def __init__(self, max_moves=10):
        self._max_moves = 10
        self._move_set = None
        self._history = []

    def new_game(self):
        assert not self._move_set
        self._move_set = set()
        self._num_moves = 0

    def end_game(self):
        game_id = len(self._history)
        self._history.append((game_id, self._move_set))
        self._move_set = None

    # Returns True indicating found duplicated game.
    def add_move(self, move):
        if self._num_moves >= self._max_moves:
            # No-op
            return False

        self._move_set.add(move)
        self._num_moves += 1
        assert len(self._move_set) == self._num_moves

        if self._num_moves != self._max_moves:
            return False

        for (old_id, old_set) in self._history:
            if old_set == self._move_set:
                print("Find duplicated game with old id:", old_id)
                return True

        return False



