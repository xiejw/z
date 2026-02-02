// vim: ft=cpp
// forge:v1
//
#include <stdlib.h>  // size_t

namespace taocp {
struct HamiltonianGraph {
      public:
        /// Number of vertices. IDs are 0, .. n-1.
        size_t n;

        ///
        size_t *ADJ;
        size_t *NBR;

        /// Stores the solution. Length is n.
        size_t *EV;
        size_t *EU;

      private:
        // Intenral buffer to support data structures. User should not care.
        size_t *mem;

      public:
        HamiltonianGraph( size_t n );
        ~HamiltonianGraph( );

      public:
        void RunAlgoH( );
};
}  // namespace taocp
