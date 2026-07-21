// Bullshit file to pass compilation, this will be replaced by a real piece of code. -- RA 2000-03-21

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#define IN_UI_LOG_C 1

#define gen_loglevel 10
#define LOG_CRITICAL 1
#define LOG_USER 2

/*** ui_log - log message ***/

void ui_log(unsigned int loglevel, const char *text, ...)
{
    va_list ap;

    if (loglevel <= gen_loglevel)
    {
        if (loglevel == LOG_CRITICAL)
            fprintf(stderr, "CRIT ");
        else if (loglevel == LOG_USER)
            fprintf(stderr, "USER ");
        else if (loglevel > LOG_USER)
            fprintf(stderr, "DEBG ");
        else
            fprintf(stderr, "---- ");
        va_start(ap, text);
        vfprintf(stderr, text, ap);
        va_end(ap);
        fprintf(stderr, "\n");
        fflush(stderr);
    }
}

/*** ui_err - log error message and quit ***/

void ui_err(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "ERR: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
    // exit(1);
}

void ui_error(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "ERROR: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}

void ui_log_verbose(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "VERBOSE LOG: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}

void ui_log_request(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "REQUEST LOG: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}

void ui_log_critical(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "CRITICAL LOG: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}

void ui_log_debug3(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "DEBUG3 LOG: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}

void ui_log_debug2(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "DEBUG2 LOG: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}
void ui_log_debug1(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "DEBUG1 LOG: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}
void ui_log_user(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "USER LOG: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}

void ui_log_normal(const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "NORMAL LOG: ");

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}

void ui_xdebug(const char *file, int line, const char *text, ...)
{
    va_list ap;

    fprintf(stderr, "DEBUG (%s:%d)", file, line);

    va_start(ap, text);
    vfprintf(stderr, text, ap);
    va_end(ap);
    putc(10, stderr);
    fflush(stderr);
}
