#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "model.h"

namespace py = pybind11;

static std::vector<int> legal_moves(const std::vector<int> &board);

// the entry point of the python module
int
select_next_move(const std::vector<int> &v, int next_player_color)
{
        // int sum = 0;
        f32_t  probs[BOARD_SIZE];
        f32_t  value;
        f32_t *probs_ptr = probs;

        // call model
        error_t err = c4_model_predict(
            /*next_player_color=*/next_player_color,
            /*board=*/v.data(),
            /*pos_count=*/BOARD_SIZE,
            /* outputs */
            /*prob=*/&probs_ptr,
            /*value=*/&value);
        assert(err == OK);

        DEBUG() << "[debug] winning probabilty from model: " << value << "\n";

        auto legal_moves_pos = legal_moves(v);

        int   pos_max = -1;
        f32_t v_max   = 0;
        for (auto i : legal_moves_pos) {
                f32_t v = probs[i];
                if (v >= v_max) {
                        pos_max = i;
                        v_max   = v;
                }
        }

        DEBUG() << "[debug] best pos for next move is " << pos_max
                << " with prob to be selected " << v_max << "\n ";

        return pos_max;
}

void
cleanup()
{
        c4_model_cleanup();
}

PYBIND11_MODULE(xai_c4, m)
{
        m.doc() = "parallel mcts algorithm";  // optional module docstring

        m.def("select_next_move", &select_next_move,
              "A function to select next pos to play");

        m.def("cleanup", &cleanup, "A function to clean up");
}

std::vector<int>
legal_moves(const std::vector<int> &board)
{
        std::vector<int> pos{};
        for (int col = 0; col < COL_COUNT; col++) {
                for (int row = ROW_COUNT - 1; row >= 0; row--) {
                        int index = row * COL_COUNT + col;
                        if (board[index] == NA_INT) {
                                pos.push_back(index);
                                break;
                        }
                }
        }
        assert(pos.size() > 0);
        return pos;
}
