#ifndef PARSE_H__INCLUDED
#define PARSE_H__INCLUDED

// Parse a command and dispatch to the appropriate function.
// Return 1 if user requested to exit.
int parseCmd(const char *cmd);

#endif // PARSE_H__INCLUDED
