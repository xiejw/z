// vim: ft=cpp
// forge:v1
//
#include <vector>

namespace taocp {
struct SGBNode {
      public:  // Initai
        std::vector<struct SGBNode *> arcs;

      public:  // Used by algorithm
        using ArcIter = std::vector<struct SGBNode *>::iterator;

        struct SGBNode *parent;
        struct SGBNode *link;
        size_t          rep;
        ArcIter         arc;
};
struct SGB {
      public:
        std::vector<SGBNode> vertices;

      public:
        void Run( );
};
}  // namespace taocp
