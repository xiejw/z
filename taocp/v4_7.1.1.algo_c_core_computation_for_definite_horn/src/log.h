// vim: ft=cpp
//
// forge:v1
//
// C++17 based Logging Library
#pragma once

#define PANIC( ... ) base::cc17::PanicImpl( __FILE__, __LINE__, __VA_ARGS__ )
#define INFO( ... )  base::cc17::InfoImpl( __FILE__, __LINE__, __VA_ARGS__ )
#define WARN( ... )  base::cc17::WarnImpl( __FILE__, __LINE__, __VA_ARGS__ )

namespace base::cc17 {
void PanicImpl( const char *file, int line, const char *fmt, ... );
void InfoImpl( const char *file, int line, const char *fmt, ... );
void WarnImpl( const char *file, int line, const char *fmt, ... );
}  // namespace base::cc17
