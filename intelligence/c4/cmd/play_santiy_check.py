#
# Code to test self play between two random policies.
#


from game import GameConfig
from policy import RandomPolicy
from play import play_games

#
# Initialize the env
#

config = GameConfig()
print(config)

players = lambda b: [RandomPolicy(b, 'b'), RandomPolicy(b, 'w')]


play_games(config, players=players)

