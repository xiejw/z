from .human    import HumanPolicy
from .mcts     import MCTSPolicy
try:
    from .mcts_par import MCTSParPolicy
except ImportError:
    print("[sys] skip importing MCTSParPolicy due to missing dependency. it is ok")

from .random   import RandomPolicy

# This must be inference mode not training or exploration mode.
BestPolicy = MCTSPolicy
