import random
import math
import copy
import os

import numpy as np

from data import InferenceState
from game import Color
from game import Move
from game import Position
from model import build_model
from model import convert_inference_state_to_model_feature

from .policy import Policy

STATE_FILE_PATH_KEY = "C4_STATE_FILE"


# Builds a model and loads weights.
def _build_model(config):
    num_classes = config.rows * config.columns
    input_shape = (config.rows, config.columns, 3)

    state_file = os.getenv(STATE_FILE_PATH_KEY)
    m = build_model(input_shape, num_classes, state_file_to_load=state_file)

    return m


class MCTSNode(object):

    # MCTSNode should own the board. Call site should make deepcopy if
    # necessary.
    def __init__(self, board, next_player_color, model, inject_noise=False):
        self.board = board
        self._config = board.config
        self._inject_noise_to_root = inject_noise
        self.next_player_color = next_player_color
        self.model = model

        self.legal_positions = board.legal_positions()
        if not self.legal_positions:
            raise RuntimeError("Board is full already. Unexpected.")

        # Stores children.
        self.c = {}

        # Stores statistic.
        self.total_count = 0

        self.n = {}
        self.w = {}
        self.p = {}

        # Fills in statistic.
        pred = self.model.predict(
                convert_inference_state_to_model_feature(
                    self._config,
                    InferenceState(
                        config=self._config,
                        # Just needs a view.
                        snapshot=board.snapshot(deepcopy=False),
                        next_player_color=self.next_player_color)))

        assert isinstance(pred, (list, tuple))
        policy_pred, value_pred = pred
        del pred

        assert policy_pred.shape, (1, self._config.rows * self._config.columns)
        assert value_pred.shape, (1, 1)

        self.predicated_reward = float(value_pred[0][0])

        for pos in self.legal_positions:
            index = self._config.convert_position_to_index(pos)
            self.p[pos] = policy_pred[0][index]
            self.n[pos] = 0
            self.w[pos] = 0

        # We inject uniform noises.
        if inject_noise:
            a = np.random.uniform(0, 1, len(self.legal_positions))
            a = a / np.sum(a)

            for (i, pos) in enumerate(self.legal_positions):
                self.p[pos] = 0.8 * self.p[pos] + 0.2 * a[i]


    def backup(self, pos, value):
        assert pos in self.n
        self.total_count += 1
        self.n[pos] += 1
        self.w[pos] += value

    # Selects a node.
    #
    # - The algorithrm is based on current simulation results.
    # - It is used for next round of simulation.
    def select_next_pos_to_evaluate(self):
        c = 1.0
        q = {}

        sqrt_total_count = math.sqrt(self.total_count)
        for pos in self.legal_positions:
            n = self.n[pos]
            q[pos] = self.w[pos] / (n if n else 1)
            q[pos] += c * self.p[pos] * sqrt_total_count / (1.0 + self.n[pos])

        max_pos = max(q, key=lambda k: q[k])
        return max_pos

    def select_next_pos_to_play(self, debug=False, explore=False):
        q = []
        for pos in self.legal_positions:
            q.append((pos, self.n[pos]))

        if debug:
            print("")
            for i, item in enumerate(
                    sorted(q, key=lambda x: x[1], reverse=True)[:5]):
                pos = item[0]
                n = item[1]
                print("  Candidate %d: %s:  -> n (%4d) p (%f) w (%f)" % (
                    i,
                    pos,
                    n,
                    self.p[pos],
                    self.w[pos] / (n if n else 1)))
            print("")

        if not explore:
            # Select the pos with maximum visited counts.
            max_pair = max(q, key=lambda x: x[1])
            return max_pair[0]

        population, weights = zip(*q)
        max_pos = random.choices(population, weights)[0]
        return max_pos


    def simulate(self, iterations):
        _run_simulations(
                iterations=iterations,
                root_node=self,
                root_board=self.board,
                inject_noise=self._inject_noise_to_root)


# A policy based on MCTS and trained model.
#
# See AlphaGo Zero paper for details.
class MCTSPolicy(Policy):


    def __init__(self, board, color, model=None, iterations=1600, explore=False, debug=False, name=None):
        self._board = board
        self._config = board.config
        self._color = Color.of(color)
        self._model = model or _build_model(self._config)
        self._iterations = iterations
        self._explore = explore
        self._inject_noise_to_root = explore
        self._debug = debug
        self.name = name if name else "mcts_" + color

        self._root = None
        self._last_known_move_count = 0

    def next_position(self):
        if self._root is None:
            # Creates the if necessary (Just started the game)
            if self._color == Color.BLACK:
                assert len(self._board.moves) == 0
            else:
                assert self._color == Color.WHITE
                assert len(self._board.moves) == 1

            new_board = copy.deepcopy(self._board)  # make a copy
            # We inject noise only for root.
            node = MCTSNode(new_board, self._color, self._model,
                            inject_noise=self._inject_noise_to_root)
            self._root = node
        else:
            # Promot next node
            moves = self._board.moves
            last_pos = moves[-1].position  # Component's move

            # In normal mcts set up, a new simulation run should be started.
            # Here we cheat a little bit to to "inherit" knowledge from previous
            # runs. So the strength should be better.
            new_root = self._root.c.get(last_pos)
            if new_root is None:
                # This case means that the move by component has never been
                # consided by this tree before.

                new_board = copy.deepcopy(self._board)  # make a copy
                new_root = MCTSNode(new_board, self._color, self._model,
                                    inject_noise=self._inject_noise_to_root)

            self._root = new_root

        # Simulation.
        root = self._root
        root.simulate(self._iterations)

        # Select
        explore = False
        if self._explore and len(self._board.moves) < 10:
            explore = True
        pos = root.select_next_pos_to_play(debug=self._debug, explore=explore)

        # Promote new root.
        new_root = root.c.get(pos)
        assert new_root is not None  # new_root might be `_game_is_over`.
        self._root = new_root

        return pos


# In the tree containing `MCTSNode`, some leafs are not needed as the game is
# over. `_game_is_over` is the placeholder for those so the node.c dict does not
# return `None`.
_game_is_over = object()


# Runs simulations.
#
# Write as free funciton for isolation.
def _run_simulations(iterations, root_node, root_board, inject_noise):

        for i in range(iterations):
            current_node = root_node
            current_board = root_board

            # Pairs of node and selection.
            selections = []

            # Backup reward for all nodes in `selections`.
            def backup_reward(b_reward, w_reward):
                for n, pos in selections:
                    if n.next_player_color == Color.BLACK:
                        n.backup(pos, b_reward)
                    else:
                        assert n.next_player_color == Color.WHITE
                        n.backup(pos, w_reward)

            # Keep play until game is over or new leaf is reached.
            while True:
                new_pos = current_node.select_next_pos_to_evaluate()

                selections.append((current_node, new_pos))

                new_move = Move(new_pos, current_node.next_player_color)
                new_board = copy.deepcopy(current_board)
                new_board.new_move(new_move)

                winner = new_board.winner_after_last_move()

                # Found winner.
                if winner is not None:
                    # This move will never be made so it should be None (not
                    # present) or just the placeholder (`_game_is_over`).
                    node_should_not_exist = current_node.c.get(new_pos)
                    assert (node_should_not_exist is None or
                            node_should_not_exist == _game_is_over)

                    # Writes a placehold. This is needed as `MCTSPolicy` will
                    # still promote new root node. We need an object there.
                    current_node.c[new_pos] = _game_is_over

                    if winner == Color.NA:  # Tie
                        black_player_reward = 0.0
                    else:
                        black_player_reward = (1.0
                                if winner == Color.BLACK else -1.0)

                    white_player_reward = black_player_reward * -1.0

                    backup_reward(black_player_reward, white_player_reward)
                    break  # End this iteration.

                # Expands new leaf
                if current_node.c.get(new_pos) is None:
                    next_player_color = current_node.next_player_color.reverse()
                    expanded_node = MCTSNode(
                            new_board,
                            next_player_color,
                            current_node.model,
                            inject_noise=inject_noise)

                    current_node.c[new_pos] = expanded_node

                    if next_player_color == Color.BLACK:
                        black_player_reward = expanded_node.predicated_reward
                    else:
                        black_player_reward = -1.0 * expanded_node.predicated_reward

                    white_player_reward = black_player_reward * -1.0
                    backup_reward(black_player_reward, white_player_reward)
                    break  # End this iteration.

                # Keeps playing in this iteration..
                current_board = new_board
                current_node = current_node.c.get(new_pos)

