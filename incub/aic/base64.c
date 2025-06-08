#include "base64.h"

#include <assert.h>

/* One time initialized lookup table to avoid switch cost during base64
 * decoding.
 *
 * See base64_lookup_tbl_init why 1 is here.
 */
static char BASE64_LOOKUP_TBL[256] = { 1 };

/* One time initialize the base64 lookup table. */
static void
base64_lookup_tbl_init( void )
{
        /* Non one means this has been initialized. */
        if ( BASE64_LOOKUP_TBL[0] != 1 ) return;

        char idx = 0;
        for ( char i = 'A'; i <= 'Z'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 );

        for ( char i = 'a'; i <= 'z'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 + 26 );

        for ( char i = '0'; i <= '9'; i++ ) {
                BASE64_LOOKUP_TBL[(int)i] = idx++;
        }
        assert( idx == 26 + 26 + 10 );

        BASE64_LOOKUP_TBL[(int)'+'] = idx++;
        BASE64_LOOKUP_TBL[(int)'/'] = idx++;
        assert( idx == 64 );

        BASE64_LOOKUP_TBL[(int)'='] = 0;

        BASE64_LOOKUP_TBL[0] = 0; /* Record init is done. */
}
/* Algorithm
 * - Check https://en.wikipedia.org/wiki/Base64 for reference.
 * - Unpack 4 chars each time. Fill the bits into the 3 chars of the
 *   output.
 * - The '=' padding is ignored as it just becomes a NULL terminator.
 */
char *
base64_decode( char *buf, size_t len )
{
        base64_lookup_tbl_init( );
        assert( len > 0 && len % 4 == 0 );
        char *output = malloc( len / 4 * 3 + 1 );

        /* Once lookup up table is initialized, all valid chars, except '=' or
         * 'A', should have non zero value. */
#define CHECK_VALID_BASE64_CH( idx )                          \
        assert( buf[( idx )] == '=' || buf[( idx )] == 'A' || \
                BASE64_LOOKUP_TBL[(int)buf[( idx )]] != 0 )

        size_t output_idx = 0;
        for ( size_t i = 0; i < len; i += 4 ) {
                CHECK_VALID_BASE64_CH( i );
                CHECK_VALID_BASE64_CH( i + 1 );
                CHECK_VALID_BASE64_CH( i + 2 );
                CHECK_VALID_BASE64_CH( i + 3 );

                char a = BASE64_LOOKUP_TBL[(int)buf[i]];
                char b = BASE64_LOOKUP_TBL[(int)buf[i + 1]];
                char c = BASE64_LOOKUP_TBL[(int)buf[i + 2]];
                char d = BASE64_LOOKUP_TBL[(int)buf[i + 3]];

                output[output_idx++] = (char)( ( a << 2 ) | ( b >> 4 ) );
                output[output_idx++] =
                    (char)( ( ( b & 0xF ) << 4 ) | ( c >> 2 ) );
                output[output_idx++] = (char)( ( ( c & 0x3 ) << 6 ) | d );
        }

#undef CHECK_VALID_BASE64_CH

        output[output_idx] = '\0';
        return output;
}

/* === --- Test Code -------------------------------------------------------- */

#ifdef AIC_TEST_H_
#include <stdio.h>
#include <string.h>

int
main( void )
{
        char *res;
        res = base64_decode( "TQ==", 4 );
        assert( 0 == strcmp( "M", res ) );
        free( res );

        res = base64_decode( "TWE=", 4 );
        assert( 0 == strcmp( "Ma", res ) );
        free( res );

        res = base64_decode( "bGlnaHQgd29yaw==", 16 );
        assert( 0 == strcmp( "light work", res ) );
        free( res );

        res = base64_decode( "bGlnaHQgd29yay4=", 16 );
        assert( 0 == strcmp( "light work.", res ) );
        free( res );

        printf( "Test passed.\n" );
        return 0;
}

#endif /* AIC_TEST_H_ */
