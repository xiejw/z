#include "gguf.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace velo::gguf {

namespace {

struct FdGuard {
        int fd{ -1 };

        ~FdGuard( )
        {
                if ( fd != -1 ) close( fd );
        }
};

zion::Expected<void *>
mmap_file( const char *fname )
{
        int fd = ::open( fname, O_RDONLY );
        if ( fd == -1 ) {
                auto err = zion::Error::io_error( );
                ZION_RETURN_ERR( err, "Failed to open gguf file {}: {}", fname,
                                 strerror( errno ) );
        }

        FdGuard gd_guard{ .fd = fd };

        struct stat s;
        if ( fstat( fd, &s ) == -1 ) {
                auto err = zion::Error::io_error( );
                ZION_RETURN_ERR( err, "Failed to stat gguf data file {}: {}",
                                 fname, strerror( errno ) );
        }

        size_t fsize = (size_t)s.st_size;
        DEBUG( "Mmap size for file {} with file size: {} ", fname, fsize );

        void *addr = mmap( NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0 );
        if ( addr == MAP_FAILED ) {
                auto err = zion::Error::io_error( );
                ZION_RETURN_ERR( err, "Failed to mmap gguf data file {}: {}",
                                 fname, strerror( errno ) );
        }

        return zion::Expected<void *>{ addr };
}

}  // namespace

zion::Expected<void>
open( )
{
        auto rc = mmap_file( WF );
        if ( !rc ) {
                ZION_RETURN_ERR( rc.error( ),
                                 "Failed to open gguf weights file." );
        }
        char *ptr = (char *)rc.value( );
        INFO( "GGUF header           : {}", std::string_view( ptr, 4 ) );
        ptr += 4;
        INFO( "GGUF version          : {}", *(u32 *)ptr );
        ptr += 4;
        INFO( "GGUF tensor_count     : {}", *(u64 *)ptr );
        ptr += 8;
        INFO( "GGUF metadata_kv_count: {}", *(u64 *)ptr );
        ptr += 8;

        // loop of metadata
        // key
        INFO( "GGUF key len: {}", *(u64 *)ptr );
        ptr += 8;
        INFO( "GGUF key str: {}", std::string_view( ptr, 20 ) );
        ptr += 20;

        return zion::Expected<void>{ };
}
}  // namespace velo::gguf
