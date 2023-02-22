#include <stdarg.h>
#include <stdio.h>

void DEBUG(const char *fmt, ...) {
#ifdef NDEBUG
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
#endif
}
