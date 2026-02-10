// === --- Common Types all ADTs Use --------------------------------------- ===
#ifndef ADT_TYPES_H_
#define ADT_TYPES_H_

// Primitive Types
#include <stdint.h>

typedef uint64_t      u64;
typedef int64_t       i64;
typedef uint32_t      u32;
typedef int32_t       i32;
typedef uint8_t       u8;
typedef float         f32;
typedef double        f64;
typedef unsigned char byte;

// Result and Error Codes
typedef int error_t;

#define OK        0
#define ERROR     -1
#define EMALLOC   -2
#define ENOTEXIST -3
#define ENOTIMPL  -4
#define EIO       -5
#define EEOF      -6
#define EINVALID  -7

// Function Parameter Annotations
#define _MUT_       // The field might be mutated if new address is allocated
#define _OUT_       // The field will be set with the output
#define _INOUT_     // The field will be passed in and then be set as output
#define _MOVED_IN_  // The ownership is moved into the method
#define _NULLABLE_  // The field is Nullable

// Function Annotations
#define ADT_UNUSED_FN         __attribute__( ( unused ) )
#define ADT_MAYBE_UNUSED( x ) (void)( x )
#define ADT_NO_DISCARD        __attribute__( ( warn_unused_result ) )

#endif /* ADT_TYPES_H_ */
