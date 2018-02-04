#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define LOG_FILENAME "/tmp/emp.log"

enum log_errcode {
    OK = 1,
    ERR_OOM,            // out of memory
    ERR_OOB,            // out of bounds
    ERR_BADFILE,
    ERR_BADOPT,         // bad option (e.g. from getopt)
    ERR_INVALID,        // invalid operation
    ERR_GFXINIT,        // couldn't initialize graphics
    ERR_GFXOP,          // we couldn't execute a specific graphic operation
};

extern FILE *log_stream();
extern int log_close();
extern void log_open(FILE *);
extern void log_write(int, const char *, ...);

/*
 * Here we have a couple of convenience macros that abstracts the log
 * level number.
 */
#define log_critical(...) log_write(0, __VA_ARGS__)
#define log_error(...) log_write(0, __VA_ARGS__)

#endif
