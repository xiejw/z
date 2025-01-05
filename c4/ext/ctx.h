#pragma once

#include <iostream>

typedef float f32_t;

typedef int error_t;
#define OK  0
#define ERR -1

#define _C4_Nullable
#define _C4_Out

#define DEBUG_ENABLED  1
#define DEBUG2_ENABLED 0

#define INFO( ) std::cout << "[info] "
#define DEBUG( ) \
        if ( DEBUG_ENABLED ) std::cout << "[debug] "

#define DEBUG2( ) \
        if ( DEBUG_ENABLED && DEBUG2_ENABLED ) std::cout << "[debug2] "

#define ERROR( ) std::cerr << "[err] "
