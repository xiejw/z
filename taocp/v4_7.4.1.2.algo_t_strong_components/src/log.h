// vim: ft=cpp
//
// forge:v2
#pragma once

#define PANIC( ... ) \
        forge::logging::cc17::PanicImpl( __FILE__, __LINE__, __VA_ARGS__ )
#define INFO( ... ) \
        forge::logging::cc17::InfoImpl( __FILE__, __LINE__, __VA_ARGS__ )
#define WARN( ... ) \
        forge::logging::cc17::WarnImpl( __FILE__, __LINE__, __VA_ARGS__ )

namespace forge::logging::cc17 {
void PanicImpl( const char *file, int line, const char *fmt, ... );
void InfoImpl( const char *file, int line, const char *fmt, ... );
void WarnImpl( const char *file, int line, const char *fmt, ... );
}  // namespace forge::logging::cc17
