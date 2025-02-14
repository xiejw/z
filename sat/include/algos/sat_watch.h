// vim: ft=cpp
#pragma once

#include <span>
#include <vector>

namespace eve::algos::sat {

using literal_t = size_t;

/* Encode the complement of a literal (1-based). */
auto C( literal_t c ) -> literal_t;

class WatchSolver {
      private:
        size_t num_emitted_causes = 0;

        std::vector<size_t> cells = { }; /* Index by cell. */
        std::vector<size_t> start = { }; /* Index by cause. */
        std::vector<size_t> link  = { }; /* Index by cause. */
        std::vector<size_t> watch = { }; /* Index by literal and cause. */

      public:
        WatchSolver( size_t num_literals, size_t num_causes );

      public:
        auto EmitCause( std::span<literal_t> ) -> void;
};
}  // namespace eve::algos::sat
