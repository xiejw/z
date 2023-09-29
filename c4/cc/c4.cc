#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "model.h"
#include "policy.h"

namespace py = pybind11;

// the entry point of the python module
int
select_next_move(const std::vector<int> &b, int next_player_color)
{
        return policy_lookahead_onestep(b, next_player_color);
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
