#ifndef AIC_UTIL_H_
#define AIC_UTIL_H_

#include <adt/types.h>
#include <adt/vec.h>

/* Returns milli seconds since epoch as double. */
double time_ms_since_epoch( void );

/* Store and load u64 into (from) bytecode. */
void bytecode_store_u64( vec_t( byte ) * pcode, u64 v );
u64  bytecode_load_u64( byte *ptr );
#endif /* AIC_UTIL_H_ */
