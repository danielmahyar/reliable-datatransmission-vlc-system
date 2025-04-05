#include "logging.h"
#include <stdarg.h>
#include <stdio.h>

void log(LogLevel status, char *message, ...)
{
    printf("%d;", status);
    va_list args;
    va_start(args, message);
    vprintf(message, args);
    va_end(args);
    printf("\n");
}