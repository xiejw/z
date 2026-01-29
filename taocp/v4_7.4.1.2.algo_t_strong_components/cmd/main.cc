#include <stdio.h>

#include <memory>

#include "graph_sgb.h"
#include "log.h"
#include "test_macros.h"

namespace {
using namespace taocp;
FORGE_TEST( test_single_node )
{
        SGBNode  v{ };
        SGBGraph g{ };
        g.vertices.push_back( std::move( v ) );

        g.RunAlgoT( );

        return NULL;
}
}  // namespace

int
main( )
{
        forge::test_suite_run( );
}
