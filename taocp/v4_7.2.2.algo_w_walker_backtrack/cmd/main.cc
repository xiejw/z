// === --- Algorithm W - Walker Backtrack - Vol 4B Page 33 ------------------
// forge:skip
//
// History:
// - [2026-01-20] V4 Avoid a_l/b_l/c_l loads from W3->W2.
// - [2026-01-20] V3 Skip l==kNumQueue computation.
// - [2026-01-20] V2 Optimize away unnecessary A/B/C reads and move s_l check.
// - [2026-01-20] V1 Optimize away S_l reading in W2 and W3.
// - [2026-01-20] V0 can work. But mem access is much higher than book reports.
//
// Table:
// - V4 Mems  6'893'407'587 Wall Clock 4.7 secs
// - V3 Mems 10'272'660'960 Wall Clock 5.2 secs
// - V2 Mems 10'331'751'008 Wall Clock 5.8 secs
// - V1 Mems 12'508'775'789 Wall Clock 6.1 secs
// - V0 Mems 14'761'611'371 Wall Clock 6.3 secs
//
#include <assert.h>
#include <inttypes.h>  // Required for PRIu64
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "log.h"  // Required for INFO

namespace {

// === --- Configuration and Auxiliary Data Structures --------------------- ===
//
// Hard code the number of Queues.
//
// kNumQueue =  8   Counter = 92
// kNumQueue = 16   Counter = 14772512
//
constexpr int kNumQueue = 8;
// constexpr int kNumQueue = 16;

// Bit vector algorithm assumes the size of register.
static_assert( kNumQueue + 1 <= sizeof( uint64_t ) * 8 );

#define PRINT_SOLUTION 0

// === --- Memory Access Macros -------------------------------------------- ===

uint64_t mem_access_counter = 0;

// #define MEM_R( x, i )    ( ( x )[( i )] )
// #define MEM_W( x, i, v ) ( ( x )[( i )] = ( v ) )
#define MEM_R( x, i )    ( mem_access_counter++, ( x )[( i )] )
#define MEM_W( x, i, v ) ( mem_access_counter++, ( x )[( i )] = ( v ) )

// === --- Count number of solutions --------------------------------------- ===

uint64_t counter = 0;

void
VisitSolution( int64_t *A )
{
        counter++;

        if ( !PRINT_SOLUTION ) return;

        int64_t X[1 + kNumQueue];

        // Deduce the t based on A based on the hint of exercise 10.
        for ( int i = 1; i <= kNumQueue; i++ ) {
                int64_t t = A[i + 1] - A[i];
                // Deduce the x from t (pos is 0 based).
                int     pos = 0;
                int64_t x   = t;
                while ( ( x & 1 ) == 0 ) {
                        x >>= 1;
                        pos++;
                }
                pos++;  // Convert 0 based to 1 based.
                X[i] = pos;

                printf( "%d -> t %3d -> pos %d\n", i, (int)t, pos );
        }

        // Plot the board with all queues.
        printf( "|" );
        for ( int i = 1; i <= kNumQueue; i++ ) {
                printf( "%d", i );
        }
        printf( "|\n" );

        for ( int i = 1; i <= kNumQueue; i++ ) {
                int64_t x = X[i];
                printf( "|" );
                for ( int col = 1; col <= kNumQueue; col++ ) {
                        printf( "%c", col == x ? 'X' : ' ' );
                }
                printf( "|\n" );
        }
        printf( "One solution is printed. Abort now so you can read.\n" );
        exit( 0 );
}

// === --- Algorithm W - Walker Backtrack - Vol 4B Page 33 ------------------
//
// The answer to the exercise is on Page 398.

void
Search( )
{
        /// Mask
        constexpr int64_t U = ( 1 << ( kNumQueue ) ) - 1;

        // All arrays start from base 1. All arrays might go to kNumQueue + 1
        // level.
        int64_t A[kNumQueue + 1 + 1] = { 0 };
        int64_t B[kNumQueue + 1 + 1] = { 0 };
        int64_t C[kNumQueue + 1 + 1] = { 0 };
        int64_t S[kNumQueue + 1 + 1] = { 0 };

        int l;

        int64_t t;    // tmp var
        int64_t a_l;  // tmp var to store A[l];
        int64_t b_l;  // tmp var to store B[l];
        int64_t c_l;  // tmp var to store C[l];
        int64_t s_l;  // tmp var to store S[l];

        goto W1;

W1:  // Initialize
        l   = 1;
        a_l = b_l = c_l = 0;

        // Fallthrough

W2:  // Enter level l
        if ( l > kNumQueue ) {
                VisitSolution( &A[0] );
                goto W4;
        }

        // Invariant.
        assert( l >= 1 && l <= kNumQueue );

        /// Update S[l]

        // NOTE:
        // There are two possible branches touching here
        // 1. From W1, a_l/b_l/c_l are initialized already.
        // 2. From W3's goto W2, where all a_l/b_l/c_l are stored in
        //    registers already.
        //
        // The following 3 MEM_R can be saved for each level advance!
        //
        // a_l = MEM_R( A, l );
        // b_l = MEM_R( B, l );
        // c_l = MEM_R( C, l );
        //
        t   = a_l | b_l | c_l;
        s_l = U & ( ~( t ) );

        /* NOTE:

           This special optimization can roughly reduce mems from 6'952'497'635
           to 6'893'407'587 mems as it skips the entire W3 for 'unnecessary'
           execution.

           In order to visit the solution X, we could deduce the final bit with

              t = s_l & ( -s_l )

           That's it. All the MEM_W in the W3 can be skipped.

           Keep the code in comment so the code looks similar to the algorithm
           in the book.
        */
        // if ( s_l != 0 && l == kNumQueue ) {
        //         l++;
        //         VisitSolution( );
        //         goto W4;
        // }

        // Fallthrough

W3:  // Try advance

        // Invariant.
        assert( l >= 1 && l <= kNumQueue );

        // NOTE: If W2 and W4 have s_l ==0 check. This branch can be removed.
        if ( s_l == 0 ) {  // S_l is empty
                goto W4;   // Backtrack
        }

        // NOTE: s_l is prepared in W2 or W4 already. So no need to load
        // memory now.
        t = s_l & ( -s_l );

        // NOTE:
        // 1. a_l/b_l/c_l are prepared in label W2 and W4 already. No need to
        //    load them in this tight loop.
        // 2. Store a_l/b_l/c_l in place so the goto W2 later can skip the load.
        //
        a_l = a_l + t;
        b_l = ( b_l + t ) >> 1;
        c_l = ( ( c_l + t ) << 1 ) & U;

        // Store them in case backtrack
        MEM_W( A, l + 1, a_l );
        MEM_W( B, l + 1, b_l );
        MEM_W( C, l + 1, c_l );

        // This is the undo work for W4. Mentioned in Exercise 10
        s_l = s_l - t;

        MEM_W( S, l, s_l );  // Store to memory for backtrack.

        l++;
        goto W2;

W4:  // Backtrack
        l--;

        if ( l == 0 ) {
                return;
        }

        assert( l > 0 );
        // No UNDO Work to do as Done in W3 already.

        // Load from memory for backtrack so both W4->W3 and W2->W3 can prepare
        // s_l already.
        s_l = MEM_R( S, l );

        // Skip checking this in W3 and goto W4 for backtrack directly.
        if ( s_l == 0 ) {
                goto W4;
        }

        // Similar to the s_l idea above, load a_l/b_l/c_l from memory  so both
        // W4->W3 and W2->W3 can prepare them  already.
        //
        a_l = MEM_R( A, l );
        b_l = MEM_R( B, l );
        c_l = MEM_R( C, l );
        goto W3;
}

}  // namespace

int
main( )
{
        INFO( "Walker's Backtrack (Vol 4B, Page 33) - N Queue: N = %d",
              kNumQueue );
        Search( );
        INFO( "Done: %" PRIu64, counter );
        INFO( "Memory Access: %" PRIu64, mem_access_counter );
}
