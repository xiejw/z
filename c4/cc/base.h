#ifndef C4_BASE_H_
#define C4_BASE_H_

#include <iostream>

typedef float f32_t;

typedef int error_t;
#define OK 0

#define DEBUG_ENABLED  1
#define DEBUG2_ENABLED 0

#define DEBUG() \
        if (DEBUG_ENABLED) std::cout

#define DEBUG2() \
        if (DEBUG_ENABLED && DEBUG2_ENABLED) std::cout

#define ERR() std::cerr

#endif  // C4_BASE_H_
