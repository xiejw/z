#include <stdio.h>

#include "log.h"
#include "test_macros.h"

namespace {
FORGE_TEST( test_new ) { return NULL; }
}  // namespace

int
main( )
{
        forge::test_suite_run( );
}
