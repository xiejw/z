from .human    import HumanPolicy
from .mcts     import MCTSPolicy
from .random   import RandomPolicy

# This must be inference mode not training or exploration mode.
BestPolicy = MCTSPolicy
