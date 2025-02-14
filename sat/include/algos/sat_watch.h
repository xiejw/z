// vim: ft=cpp
#pragma once

#include <span>
#include <vector>

namespace eve::algos::sat {
class WatchSolver {
      private:
        size_t num_emitted_causes = 0;

        std::vector<size_t> cells = { }; /* Index by cell. */
        std::vector<size_t> start = { }; /* Index by cause. */
        std::vector<size_t> link  = { }; /* Index by cause. */
        std::vector<size_t> watch = { }; /* Index by literal. */

      public:
        auto ReserveCells( size_t cap ) -> void { this->cells.reserve( cap ); }
        auto ReserveCauses( size_t cap ) -> void;
        auto EmitCause( std::span<size_t> ) -> void;
};
}  // namespace eve::algos::sat
