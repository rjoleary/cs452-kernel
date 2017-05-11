#ifndef PANIC_H__INCLUDED
#define PANIC_H__INCLUDED

#define PANIC() panic(__FILE__, __LINE__)
void panic(const char *file, int line);

#endif // PANIC_H__INCLUDED
