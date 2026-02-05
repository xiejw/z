// vim: ft=cpp
// forge:v1
//
// === --- 2SAT Satisfiability Vol 4A 7.1.1 Theorm K Page 62 --------------- ===
//
// === --- TLDR
//
// 2SAT problem is AND of Krom clauses. Each Krom clause is an OR of at most
// two literals. Its satisfiability can be concluded with linear time
// algorithm.
//
// Key Ideas:
//
//     Form 2SAT as directed graph. Then, partition the graph with strong
//     components. Leverage Theorem K to conclude satisfiability. Then use
//     Exercise 54 (Page 86) to check the condition in each strong component
//     with linear time.
//
// === --- Dependencies
//
// The code base uses strong component algorithm.
//
#include "graph_sgb.h"
#include "log.h"

namespace taocp {

struct TwoSatSolver {
      private:
        size_t   n;
        SGBGraph g;

      public:
        /// Prepare a solver for 2SAT with num_vars of variables. Internally, 2
        /// num_vars variables will be prepared, one for the variable and one
        /// for the complement.
        TwoSatSolver( size_t num_vars );

      public:
        /// Add a Krom clause (v_id || u_id). For complement for a variable v,
        /// use GetComplementId(v) as input.
        void AddKromClause( size_t v_id, size_t u_id );

        /// Returns true if the 2SAT is satisfiable.
        bool CheckSatisfiability( );

        /// Return an ID as complement of var_id.
        ///
        /// Invariant: GetComplementId(GetComplementId(v)) == v.
        size_t GetComplementId( size_t var_id )
        {
                return var_id >= n ? var_id - n : var_id + n;
        }

        /// Return an canonical ID of var_id so it is not complement and can be
        /// used as a canonical ID.
        ///
        /// Invariant:
        ///   GetCanonicalId(v) = GetCanonicalId(GetComplementId(v))
        size_t GetCanonicalId( size_t var_id )
        {
                return var_id >= n ? var_id - n : var_id;
        }
};

}  // namespace taocp
