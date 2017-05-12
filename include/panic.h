#ifndef PANIC_H__INCLUDED
#define PANIC_H__INCLUDED

#define PANIC(str) panic(str, __FILE__, __LINE__)
void panic(const char *str, const char *file, int line);

#endif // PANIC_H__INCLUDED
