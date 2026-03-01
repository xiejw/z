// vim: ft=cpp
//
// forge:v1
#pragma once

#define PANIC( ... ) base::PanicImpl( __FILE__, __LINE__, __VA_ARGS__ )
#define INFO( ... )  base::InfoImpl( __FILE__, __LINE__, __VA_ARGS__ )
#define WARN( ... )  base::WarnImpl( __FILE__, __LINE__, __VA_ARGS__ )

namespace base {
void PanicImpl( const char *file, int line, const char *fmt, ... );
void InfoImpl( const char *file, int line, const char *fmt, ... );
void WarnImpl( const char *file, int line, const char *fmt, ... );
}  // namespace base
