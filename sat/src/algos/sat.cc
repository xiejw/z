#include <algos/sat.h>

#include <print>

namespace eve::algos::sat {

namespace {
constexpr size_t mask = 1 << ( sizeof( size_t ) - 1 );
}

auto
LiteralRawValue( literal_t c ) -> literal_t
{
        return c & ( ~mask );
}

auto
LiteralIsC( literal_t c ) -> bool
{
        return c & mask;
}

auto
C( literal_t c ) -> literal_t
{
        return c | mask;
}

auto
PrintLiterals( std::span<const literal_t> lits ) -> void
{
        if ( lits.empty( ) ) {
                std::print( "(empty literals)\n" );
                return;
        }

        std::print( "< " );
        for ( auto i : lits ) {
                if ( LiteralIsC( i ) )
                        std::print( "C({:3}), ", LiteralRawValue( i ) );
                else
                        std::print( "{:3}, ", i );
        }
        std::print( " >\n" );
}
}  // namespace eve::algos::sat
