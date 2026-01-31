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
        // === --- User Inputs --------------------------------------------- ===
      public:
        SGBGraph( size_t num_vertices ) : num_vertices_expected( num_vertices )
        {
                // Ensure the points are stable.
                vertices.reserve( num_vertices );
        }

        // === --- Internal Data Structures -------------------------------- ===

      private:
        std::vector<SGBNode> vertices;
        size_t               num_vertices_expected;

      private:
        // See RunAlgoT.
        std::vector<int> component_ids;

        // === --- Public APIs --------------------------------------------- ===

      public:
        SGBNode *AppendVertex( )
        {
                assert( this->vertices.size( ) + 1 <=
                        this->num_vertices_expected );
                this->vertices.emplace_back( );
                return &this->vertices.back( );
        }

        // Run Algorithm T (V4F12A Strong Components) and then
        // GetComponentIdsAfterAlgotT can be used.
        void RunAlgoT( );

        // Return the component ids corresponding to the vertices, one id for
        // each vertex. The value domain for id is not defined. But it is
        // guaranteed that
        //
        //     0<= id < vertices.size();
        const std::vector<int> *GetComponentIdsAfterAlgoT( ) const
        {
                return &component_ids;
        };
};
}  // namespace taocp
