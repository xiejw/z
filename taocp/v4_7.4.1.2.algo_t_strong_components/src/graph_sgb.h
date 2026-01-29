// vim: ft=cpp
// forge:v1
//
#include <vector>

namespace taocp {
struct SGBNode {
      public:  // Initialized by users.
        std::vector<struct SGBNode *> arcs;

      public:
        SGBNode( ) = default;

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
      public:
        std::vector<SGBNode> vertices;

      public:
        void RunAlgoT( );
};
}  // namespace taocp
