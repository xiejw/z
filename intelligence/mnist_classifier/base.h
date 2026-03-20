/* base.h — forge_ platform utilities */
#ifndef BASE_H
#define BASE_H

#include <stddef.h>

/* Returns the number of logical CPUs available, or 1 on failure. */
size_t forge_n_logical_cpus( void );

#endif /* BASE_H */
