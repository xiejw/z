// === --- Algorithm W - Walker Backtrack - Vol 4B Page 33 ------------------
//
// History:
// - [2026-01-20] V1 Optimize away S_l reading in W2 and W3.
// - [2026-01-20] V0 can work. But mem access is much higher than book reports.
//
// Table:
// - V1 Mems 12'508'775'789 Wall Clock 6.1 secs
// - V0 Mems 14'761'611'371 Wall Clock 6.3 secs
//
#include <assert.h>
#include <inttypes.h>  // Required for PRIu64
#include <stdint.h>
#include <stdio.h>

#include "log.h"  // Required for INFO

namespace {

// === --- Configuration and Auxiliary Data Structures --------------------- ===
//
// Hard code the number of Queues.
//
// kNum =  8   Counter = 92        MEM Acc Counter = 26'273
// kNum = 16   Counter = 14772512  MEM Acc Counter = 14'761'611'371
//
// constexpr int kNum = 8;
constexpr int kNum = 16;

// Bit vector algorithm assumes the size of register.
static_assert( kNum + 1 <= sizeof( uint64_t ) * 8 );

// === --- Memory Access Macros -------------------------------------------- ===

uint64_t mem_access_counter = 0;

// #define MEM_R( x, i )    ( ( x )[( i )] )
// #define MEM_W( x, i, v ) ( ( x )[( i )] = ( v ) )
#define MEM_R( x, i )    ( mem_access_counter++, ( x )[( i )] )
#define MEM_W( x, i, v ) ( mem_access_counter++, ( x )[( i )] = ( v ) )

// === --- Count number of solutions --------------------------------------- ===

uint64_t counter = 0;

void
VisitSolution( )
{
        counter++;
}

// === --- Algorithm W - Walker Backtrack - Vol 4B Page 33 ------------------
//
// The answer to the exercise is on Page 398.

void
Search( )
{
        /// Mask
        constexpr int64_t U = ( 1 << ( kNum ) ) - 1;

        // All arrays start from base 1. All arraries might to to kNum + 1
        // level.
        int64_t A[kNum + 1 + 1] = { 0 };
        int64_t B[kNum + 1 + 1] = { 0 };
        int64_t C[kNum + 1 + 1] = { 0 };
        int64_t S[kNum + 1 + 1] = { 0 };

        int l;

        int64_t t;    // tmp var
        int64_t s_l;  // tmp var to store S[l];

        goto W1;

W1:  // Initialize
        l = 1;

        // Fallthrough

W2:  // Enter level l
        if ( l > kNum ) {
                VisitSolution( );
                goto W4;
        }

        // Invariant.
        assert( l >= 1 && l <= kNum );

        /// Update S[l]
        {
                t   = MEM_R( A, l ) | MEM_R( B, l ) | MEM_R( C, l );
                s_l = U & ( ~( t ) );
        }

        // Fallthrough

W3:  // Try advance

        // Invariant.
        assert( l >= 1 && l <= kNum );

        // s_l is read from W2 or W4.

        if ( s_l == 0 ) {  // S_l is empty
                goto W4;
        }

        t = s_l & ( -s_l );
        MEM_W( A, l + 1, MEM_R( A, l ) + t );
        MEM_W( B, l + 1, ( MEM_R( B, l ) + t ) >> 1 );
        MEM_W( C, l + 1, ( ( MEM_R( C, l ) + t ) << 1 ) & U );

        s_l = s_l - t;       // This is the undo work for W4
                             //
        MEM_W( S, l, s_l );  // Store to memory for backtrack.

        l++;
        goto W2;

W4:  // Backtrack
        l--;

        if ( l > 0 ) {
                // No UNDO Work to do as Done in W3 already.

                s_l = MEM_R( S, l );  // Load from memory for backtrack.
                goto W3;
        }
        // Fallthrough

        // Exit
        return;
}

}  // namespace

int
main( )
{
        INFO( "Walker's Backtrack (Vol 4B, Page 33) - N Queue: N = %d", kNum );
        Search( );
        INFO( "Done: %" PRIu64, counter );
        INFO( "Memory Access: %" PRIu64, mem_access_counter );
}
