#include "sat.h"

namespace eos::sat {

namespace {
constexpr size_t mask = 1 << ( sizeof( size_t ) - 1 );
}

auto
decode_literal_raw_value( literal_t c ) -> literal_t
{
        return c & ( ~mask );
}

auto
is_literal_C( literal_t c ) -> bool
{
        return c & mask;
}

auto
C( literal_t c ) -> literal_t
{
        return c | mask;
}

auto
print_clause_literals( size_t size, const literal_t *lits ) -> void
{
        if ( size == 0 ) {
                printf( "(empty literals)\n" );
                return;
        }

        printf( "< " );
        for ( size_t x = 0; x < size; x++ ) {
                auto i = lits[x];
                if ( is_literal_C( i ) )
                        printf( "C(%3d), ",
                                (int)decode_literal_raw_value( i ) );
                else
                        printf( "%3d, ", int( i ) );
        }
        printf( " >\n" );
}
}  // namespace eos::sat
