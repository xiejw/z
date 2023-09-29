#
# real fun code starts.
#

import random
import argparse

from game import GameConfig
from policy import HumanPolicy
from policy import MCTSPolicy
from play import play_games

#
# configuration to change
#

SHUFFLE_PLAYERS = True

parser = argparse.ArgumentParser()
parser.add_argument("--par",
                    action="store_true",
                    help="use parallel MCTS algorithm (c ext)")
args = parser.parse_args()

#
# initialize the env
#

config = GameConfig()
print(config)

def BestPolicy(b, c):
    if not args.par:
        print("[sys] use MCTSPolicy as agent.")
        return MCTSPolicy(b, c, explore=False, debug=True)
    else:
        from policy import MCTSParPolicy
        print("[sys] use MCTSParPolicy as agent.")
        return MCTSParPolicy(b, c)


if SHUFFLE_PLAYERS and random.random() < 0.5:
    players = lambda b: [HumanPolicy(b, 'b'), BestPolicy(b, 'w')]
else:
    players = lambda b: [BestPolicy(b, 'b'), HumanPolicy(b, 'w')]


play_games(config, players=players)

