// vim: ft=cpp
// forge:v1
//
#include <vector>

#include <assert.h>

namespace taocp {
struct SGBNode {
        // === --- User Inputs --------------------------------------------- ===

      public:  // Initialized by users.
        std::vector<struct SGBNode *> arcs;

      public:
        SGBNode( ) = default;

        // === --- Internal Data Structures -------------------------------- ===

      public:  // Used by algorithm. No need to initialize.
        using ArcIter = std::vector<struct SGBNode *>::iterator;

        struct SGBNode *parent;  // Parent node.
        ArcIter         arc;     // Store the state of next arc.

        // Dual usage. A) Stack link B) See Vol F12A Page 10-(11)
        struct SGBNode *link;

        // Dual usage: A) LOW B) See Vol F12A Page 10-(12)
        size_t rep;
};

struct SGBGraph {
        // === --- Internal Data Structures -------------------------------- ===

      private:
        std::vector<SGBNode> vertices;
#ifndef NDEBUG
        size_t num_vertices_expected;
#endif

      private:
        // See RunAlgoT.
        std::vector<int> component_ids;

        // === --- User Inputs --------------------------------------------- ===
      public:
        // Ensure the points are stable.  So vertices is allocated.
        SGBGraph( size_t num_vertices )
            : vertices( num_vertices )
#ifndef NDEBUG
              ,
              num_vertices_expected( num_vertices )
#endif
        {
        }

        // === --- Public APIs --------------------------------------------- ===
      public:
        /// Return the SGBNode at position i.
        SGBNode *GetVertex( size_t i ) { return &this->vertices.at( i ); }

        /// Run Algorithm T (V4F12A Strong Components) and then
        /// GetComponentIdsAfterAlgotT can be used.
        void RunAlgoT( );

        /// Return the component ids corresponding to the vertices, one id for
        /// each vertex. The value domain for id is not defined. But it is
        /// guaranteed that
        ///
        ///     0<= id < vertices.size();
        const std::vector<int> *GetComponentIdsAfterAlgoT( ) const
        {
                return &component_ids;
        };
};
}  // namespace taocp
