// === --- Algorithm B - Basic Backtrack - Vol 4B Page 32 ------------------ ===
//
#include <inttypes.h>  // Required for PRIu64
#include <stdint.h>
#include <stdio.h>

#include "log.h"  // Required for INFO

namespace {

// === --- Configuration and Auxiliary Data Structures --------------------- ===
//
// Hard code the number of Queues.
//
// kNum =  8   Counter = 92        MEM Acc Counter = 41'096
// kNum = 16   Counter = 14772512  MEM Acc Counter = 33'859'294'165
//
constexpr int kNum = 8;
// constexpr int kNum = 16;

// Static allocating the data structures. All arrays start from base 1.
int  X[kNum + 1]         = { 0 };
char A[kNum + 1]         = { 0 };
char B[2 * kNum - 1 + 1] = { 0 };
char C[2 * kNum - 1 + 1] = { 0 };

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

// === --- Algorithm B - Basic Backtrack - Vol 4B Page 32 ------------------ ===
void
Search( )
{
        int t;
        int l;

        goto B1;

B1:  // Initialize
        l = 1;

        // Fallthrough

B2:  // Enter level l
        if ( l > kNum ) {
                VisitSolution( );
                goto B5;
        }

        // Scan domain now.
        t = 1;

        // Fallthrough

B3:  // Try t
        if ( MEM_R( A, t ) || MEM_R( B, t + l - 1 ) ||
             MEM_R( C, t - l + kNum ) ) {
                goto B4;
        }

        MEM_W( A, t, 1 );
        MEM_W( B, t + l - 1, 1 );
        MEM_W( C, t - l + kNum, 1 );
        MEM_W( X, l, t );
        l++;
        goto B2;

B4:  // Try next t
        if ( t < kNum ) {
                t++;
                goto B3;
        }

        // Fallthrough

B5:  // Backtrack
        l--;
        if ( l > 0 ) {
                t = MEM_R( X, l );
                MEM_W( C, t - l + kNum, 0 );
                MEM_W( B, t + l - 1, 0 );
                MEM_W( A, t, 0 );
                goto B4;
        }

        // Fallthrough

        // Exit
        return;
}

}  // namespace

int
main( )
{
        INFO( "Basic Backtrack (Vol 4B, Page 32) - N Queue: N = %d", kNum );
        Search( );
        INFO( "Done: %" PRIu64, counter );
        INFO( "Memory Access: %" PRIu64, mem_access_counter );
}
