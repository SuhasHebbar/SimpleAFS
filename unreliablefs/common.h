#include <stdarg.h>
#include <stdio.h>

#ifdef NDEBUG
#define D(x)
#else
#define D(x) x
#endif

static void DEBUG(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  D(vprintf(fmt, args));
  va_end(args);
}
