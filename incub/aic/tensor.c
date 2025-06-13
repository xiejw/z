#include "tensor.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef NDEBUG
#define DEBUG_PRINT 1
#else
#define DEBUG_PRINT 0
#endif

#define DEBUG( ctx, ... ) \
        if ( DEBUG_PRINT ) LOG_DEBUG( ctx, __VA_ARGS__ )

static error_t
mmap_file( struct ctx *ctx, const char *fname, _OUT_ void **paddr )
{
        int fd = open( fname, O_RDONLY );
        if ( fd == -1 ) {
                EMIT_ERROR_NOTE( ctx, "failed to open tensor data file %s: %s",
                                 fname, strerror( errno ) );
                return EIO;
        }

        error_t err = OK;

        struct stat s;
        if ( fstat( fd, &s ) == -1 ) {
                EMIT_ERROR_NOTE( ctx, "failed to stat tensor data file %s: %s",
                                 fname, strerror( errno ) );
                err = EIO;
                goto cleanup;
        }

        size_t fsize = (size_t)s.st_size;
        DEBUG( ctx, "mmap size for file %s is %zu", fname, fsize );

        void *addr = mmap( NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0 );
        if ( addr == MAP_FAILED ) {
                EMIT_ERROR_NOTE( ctx, "failed to mmap tensor data file %s: %s",
                                 fname, strerror( errno ) );
                err = EIO;
                goto cleanup;
        }

        *paddr = addr;

cleanup:
        close( fd );
        return err;
}

static error_t
load_tensors( struct ctx *ctx, char *addr,
              _OUT_ vec_t( struct tensor * ) * ptensors )
{
        u32 count = *(u32 *)addr;
        addr += 4;
        DEBUG( ctx, "tensor count %d", (int)count );

        assert( count == 1 );
        u32 dim = *(u32 *)addr;
        addr += 4;
        DEBUG( ctx, "tensor dim %d", (int)dim );

        u32 e0 = *(u32 *)addr;
        addr += 4;
        u32 e1 = *(u32 *)addr;
        addr += 4;
        DEBUG( ctx, "tensor shape [ %d, %d ]", (int)e0, (int)e1 );

        (void)ptensors;
        return OK;
}

error_t
tsr_load_from_file( struct ctx *ctx, const char *fname,
                    _OUT_ vec_t( struct tensor * ) * ptensors )
{
        void   *addr = NULL;
        error_t err  = mmap_file( ctx, fname, &addr );
        if ( err != OK ) {
                goto cleanup;
        }

        err = load_tensors( ctx, addr, ptensors );
        if ( err != OK ) {
                goto cleanup;
        }

cleanup:
        return err;
}
