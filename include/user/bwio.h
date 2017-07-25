/*
 * bwio.h
 */
#pragma once

#include "types.h"

typedef char *va_list;

#define __va_argsiz(t)	\
		(((sizeof(t) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(ap, pN) ((ap) = ((va_list) __builtin_next_arg(pN)))

#define va_end(ap)	((void)0)

#define va_arg(ap, t)	\
		 (((ap) = (ap) + __va_argsiz(t)), *((t*) (void*) ((ap) - __va_argsiz(t))))

#define COM1	0
#define COM2	1

#define ON	1
#define	OFF	0

int bwsetfifo( int channel, int state );

int bwsetspeed( int channel, int speed );

int bwputc( int channel, char c );

void flush( int channel );

int bwgetc( int channel );

int bwputx( int channel, char c );

int bwputstr( int channel, const char *str );

int bwputr( int channel, unsigned int reg );

void bwputw( int channel, int n, char fc, char *bf );

void bwprintf( int channel, const char *format, ... );

extern bool useBusyWait;
extern ctl::Tid bwioServs[2];

// Set position of cursor to (row, col).
inline void setpos(unsigned row, unsigned col) {
    bwprintf(COM2, "\033[%d;%dH", row, col);
}

// Save cursor position.
inline void savecur() {
    bwputstr(COM2, "\033[s\033[?25l");
}

// Restore cursor position.
inline void restorecur() {
    bwputstr(COM2, "\033[u\033[?25h");
}

// Clear from cursor to end of line.
inline void clearline() {
    bwputstr(COM2, "\033[K");
}

// Write a formatted message at the given row.
#define INFOF(row, fmt, ...)                \
        savecur();                          \
        setpos(row, 1);                     \
        clearline();                        \
        bwprintf(COM2, fmt, ##__VA_ARGS__); \
        restorecur();                       \
        flush(COM2);
