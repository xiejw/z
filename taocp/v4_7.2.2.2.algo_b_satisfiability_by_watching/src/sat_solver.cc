#include "sat.h"

#include <print>

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
print_clause_literals( std::span<const literal_t> lits ) -> void
{
        if ( lits.empty( ) ) {
                std::print( "(empty literals)\n" );
                return;
        }

        std::print( "< " );
        for ( auto i : lits ) {
                if ( is_literal_C( i ) )
                        std::print( "C({:3}), ",
                                    decode_literal_raw_value( i ) );
                else
                        std::print( "{:3}, ", i );
        }
        std::print( " >\n" );
}
}  // namespace eos::sat
