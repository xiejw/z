#include <inttypes.h>  // Required for PRIu64
#include <stdint.h>
#include <stdio.h>

#include "log.h"

namespace {
constexpr int kNum = 8;

int  X[kNum + 1]     = { 0 };
char A[kNum + 1]     = { 0 };
char B[2 * kNum - 1] = { 0 };
char C[2 * kNum - 1] = { 0 };

uint64_t counter = 0;

void
VisitSolution( )
{
        counter++;
}

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
        if ( A[t] || B[t + l - 1] || C[t - l + kNum] ) {
                goto B4;
        }

        A[t]            = 1;
        B[t + l - 1]    = 1;
        C[t - l + kNum] = 1;
        X[l]            = t;
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
                t               = X[l];
                C[t - l + kNum] = 0;
                B[t + l - 1]    = 0;
                A[t]            = 0;
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
}
