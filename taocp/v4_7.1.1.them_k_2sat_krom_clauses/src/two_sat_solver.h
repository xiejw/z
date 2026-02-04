// vim: ft=cpp
// forge:v1
//
// === --- 2SAT Satisfiability Vol 4A 7.1.1 Theorm K Page 62 --------------- ===
//
// === --- TLDR
//
// Key Ideas:
//
//     Form 2SAT as directed graph. Then, partition the graph with strong
//     compnents. Leverage Theorm K to conclude satisfiability.
//
// === --- Dependencies
//
// The code base uses strong component algorithm.
//
#include "graph_sgb.h"
#include "log.h"

#define DEBUG 0
#define DEBUG_PRINTF \
        if ( DEBUG ) INFO

namespace taocp {

struct TwoSatSolver {
      private:
        size_t   n;
        SGBGraph g;

      public:
        TwoSatSolver( size_t num_vars );

      public:
        void AddKromClause( size_t v_id, size_t u_id );
        bool CheckSatisfiability( );

        size_t GetCompVarId( size_t var_id )
        {
                return var_id >= n ? var_id - n : var_id + n;
        }
        size_t GetRawVarId( size_t var_id )
        {
                return var_id >= n ? var_id - n : var_id;
        }
};

}  // namespace taocp
