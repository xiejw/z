#include "tensor.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

        void *addr = mmap( NULL, fsize, PROT_NONE, MAP_PRIVATE, fd, 0 );
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
load_tensors( struct ctx *ctx, void *addr,
              _OUT_ vec_t( struct tensor * ) tensors )
{
        return OK;
}

error_t
tsr_load_from_file( struct ctx *ctx, const char *fname,
                    _OUT_ vec_t( struct tensor * ) tensors )
{
        void   *addr = NULL;
        error_t err  = mmap_file( ctx, fname, &addr );
        if ( err != OK ) {
                goto cleanup;
        }
        (void)tensors;

cleanup:
        return err;
}
