#
# real fun code starts.
#

import random

from game import GameConfig
from policy import HumanPolicy
from policy import MCTSParPolicy
from play import play_games

#
# configuration to change
#

SHUFFLE_PLAYERS = True

#
# initialize the env
#

config = GameConfig()
print(config)

# def BestPolicy(b, c):
#     return MCTSPolicy(b, c, explore=False, debug=True)

def BestPolicy(b, c):
    return MCTSParPolicy(b, c)


if SHUFFLE_PLAYERS and random.random() < 0.5:
    players = lambda b: [HumanPolicy(b, 'b'), BestPolicy(b, 'w')]
else:
    players = lambda b: [BestPolicy(b, 'b'), HumanPolicy(b, 'w')]


play_games(config, players=players)

