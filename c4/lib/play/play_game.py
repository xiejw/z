from data import ExperienceBuffer
from game import Color
from game import Move

from .dup_detector import DupDetector


# Plays the game for `num_epochs` iterations.
#
# - `players` is a fn returning two players. First is for black stone, and second
#    is for white stone. It will be invoked for each iteration.
#
# - `writer` is passed to ExperienceBuffer directly.
def play_games(config, players, num_epochs=1, writer=None, avoid_dup=False):
    ebuf = ExperienceBuffer(config, writer=writer)

    dup_detector = DupDetector()

    i = 0
    while i < num_epochs:
        if num_epochs != 1:
            print("========================")
            print("Epoch: %3d/%d" % (i+1, num_epochs))

        b = config.new_board()
        b.draw()

        black_policy, white_policy = players(b)

        ebuf.start_epoch()

        dup_detector.new_game()
        aborted = False

        color = 'b'
        winner = None
        while True:

            policy = black_policy if color == 'b' else white_policy
            print("\n==> Inquiry", policy.name)

            position = policy.next_position()
            row, column = position.x, position.y

            print("Placed at (%2d, %2d)" % (row, column))
            move = Move((row, column), color)
            ebuf.add_move(move)
            b.new_move(move)
            b.draw()

            found_dup = dup_detector.add_move(move)
            if avoid_dup and found_dup:
              aborted = True
              break

            winner = b.winner_after_last_move()
            if winner == None:
                pass
            elif winner == Color.NA:
                print("Tie")
                break
            else:
                print("Found winner: %s" % winner)
                break

            color = 'w' if color == 'b' else 'b'

        dup_detector.end_game()

        if not aborted:
            ebuf.end_epoch(winner)
            ebuf.report()
            i += 1
        else:
            print("Abort the game as it is a dup.")
            ebuf.abort_epoch()

    ebuf.summary()

    return ebuf.history()
