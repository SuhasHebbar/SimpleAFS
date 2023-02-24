#include <stdarg.h>
#include <stdio.h>

static void DEBUG(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}
