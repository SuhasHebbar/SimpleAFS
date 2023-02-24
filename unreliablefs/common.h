#include <stdarg.h>
#include <stdio.h>

static void DEBUG(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
#ifdef NDEBUG
  vprintf(fmt, args);
#endif
  va_end(args);
}
