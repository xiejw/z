// vim: ft=cpp
//
// A non-thread-safe Error to record diagnose notes when Error occurs. The
// call stacks can record more context, i.e., more notes, to the same
// Error. It is typically used as standalone or with std::expected.
//
// Best Practice:
//
// 1. Use zion::Expected as alias of std::Expected<..., zion::Error>
//
// 2. Use ZION_EMIT_DIAG_NOTE with zion::Error to emit diagnose notes in place.
//
// 3. Use ZION_CHECK_OK_OR_RETURN with std::expected to emit diagnose notes and
//    return if error occurs.
//
// 4. Use ZION_RETURN_ERR with zion::Error or std::expected<>.error() to emit
//    diagnose notes and return unconditionally.
//
// Examples:
//
// ```
// # Standalone 1
// zion::Error
// emit_root_note( )
// {
//         auto err = zion::Error::runtime_error( );
//         ZION_EMIT_DIAG_NOTE( err, "try to emit diag note: {}", 123 );
//         return err;
// }
//
// # Standalone 2
// std::expected<void, zion::Error>
// emit_root_note( )
// {
//         auto err = zion::Error::runtime_error( );
//         ZION_RETURN_ERR( err, "try to emit diag note: {}", 123 );
// }
//
// # With std::expected
// zion::Expected<void>
// emit_note( )
// {
//         auto rc = emit_root_note( ); // std::expected<void, zion::Error>
//         ...
//         // upon error
//         ZION_EMIT_DIAG_NOTE( rc.error( ), "more context:     {}", 456 );
//         ZION_CHECK_OK_OR_RETURN( rc, "should return" );
//         return zion::Expected<void>{ };
// }
// ```
//
#pragma once

#include <cassert>
#include <expected>
#include <memory>

#include "lang.h"
#include "log.h"

namespace zion {

/* An Error records diagnose notes when error occurs. This is not thread safe
 * in general.
 */
class [[nodiscard]] Error {
      public:
        enum class Kind {
                /* Result codes */
                EndOfFile,

                /* Error codes */
                IOError,
                LengthError,
                RuntimeError,
        };

      private:
        Kind                         kind_;
        std::unique_ptr<std::string> error_msg_;  // Small mem footprint.

      private:
        static constexpr const char *fmt =
            "[F " ZION_COLOR_CYAN "{:<8}" ZION_COLOR_RESET "/L " ZION_COLOR_YELW
            "{:<4}" ZION_COLOR_RESET "] {}";

      public:
        Error( Kind kind ) : kind_( kind ) {};

        ZION_DECLARE_MOVE( Error );

        Error( const Error &err )
        {
                kind_ = err.kind_;
                if ( err.error_msg_.get( ) != nullptr ) {
                        error_msg_.reset( new std::string{ *err.error_msg_ } );
                }
        }

        Error &operator=( const Error &err )
        {
                kind_ = err.kind_;
                if ( err.error_msg_.get( ) != nullptr ) {
                        error_msg_.reset( new std::string{ *err.error_msg_ } );
                }
                return *this;
        }

      public:
        static auto runtime_error( ) { return Error{ Kind::RuntimeError }; }
        static auto io_error( ) { return Error{ Kind::IOError }; }

      public:
        /* Get the Error kind. */
        Kind get_kind( ) const { return kind_; };
        /* Get the diagnose notes. */
        const std::string &get_diag_notes( ) const { return *error_msg_; };

      public:
        /* Low level API to emit a diagnose note.
         *
         * Use ZION_EMIT_ERROR_NOTE helper macro for common use cases.
         */
        void emit_diag_note( std::string msg, const char *file, int line )
        {
                bool first_msg = error_msg_.get( ) == nullptr;
                if ( first_msg ) {
                        error_msg_.reset( new std::string{ } );
                }
                bool end_with_newline =
                    msg.size( ) > 0 && msg.data( )[msg.size( ) - 1] == '\n';
                if ( first_msg )
                        error_msg_->append(
                            std::format( fmt, file, line, "|> " ) );
                else
                        error_msg_->append(
                            std::format( fmt, file, line, "~~> " ) );

                error_msg_->append( std::move( msg ) );
                if ( !end_with_newline ) error_msg_->push_back( '\n' );
        }
};

static_assert( sizeof( Error ) <= 16, "Error size is too large" );

#define ZION_EMIT_DIAG_NOTE( err, ... ) \
        ( err ).emit_diag_note( std::format( __VA_ARGS__ ), __FILE__, __LINE__ )

template <class T>
using Expected = std::expected<T, Error>;

#define ZION_CHECK_OK_OR_RETURN( expect, ... ) \
        _ZION_CHECK_OK_OR_RETURN( expect, __FILE__, __LINE__, __VA_ARGS__ )

#define _ZION_CHECK_OK_OR_RETURN( expect, file, lineno, ... )                  \
        do {                                                                   \
                if ( !bool( expect ) ) {                                       \
                        ( expect.error( ) )                                    \
                            .emit_diag_note( std::format( __VA_ARGS__ ), file, \
                                             lineno );                         \
                        return std::unexpected{                                \
                            std::move( expect.error( ) ) };                    \
                }                                                              \
        } while ( 0 )

#define ZION_RETURN_ERR( err, ... ) \
        _ZION_RETURN_ERR( err, __FILE__, __LINE__, __VA_ARGS__ )

#define _ZION_RETURN_ERR( err, file, lineno, ... )                        \
        do {                                                              \
                ( err ).emit_diag_note( std::format( __VA_ARGS__ ), file, \
                                        lineno );                         \
                return std::unexpected{ std::move( err ) };               \
        } while ( 0 )

}  // namespace zion
