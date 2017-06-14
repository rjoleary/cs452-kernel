#include <bwio.h>
#include <train.h>
#include <std.h>

void printHelp() {
    bwputstr(COM2,
        "Assignment 0 commands:\r\n"
        "   com i [BYTE...]  - Send arbitrary byte over COMi.\r\n" // extra command
        "   help             - Display this help information.\r\n" // extra command
        "   li NUMBER        - Toggle train lights.\r\n"           // extra command
        "   q                - Quit and return to RedBoot.\r\n"
        "   rv NUMBER        - Reverse the direction of the train.\r\n"
        "   sw NUMBER DIR    - Set switch direction ('S' or 'C').\r\n"
        "   tr NUMBER SPEED  - Set train speed (0 for stop).\r\n"
    );
}

typedef struct {
    const char *start;
    unsigned len;
} Token;

typedef struct {
    Token token;
    int val;
    unsigned err; // 0 - success, 1 - error
} DecimalToken;

// Retrieve the next token from char*.
Token nextToken(const char **cmd) {
    Token t = {*cmd, 0};
    // Strip leading whitespace.
    while (**cmd == ' ' || **cmd == '\t') {
        if (**cmd == '\0') {
            return t;
        }
        (*cmd)++;
    }
    t.start = *cmd;

    // Determine size.
    while (**cmd != ' ' && **cmd != '\t' && **cmd != '\0') {
        (*cmd)++;
    }
    t.len = *cmd - t.start;
    return t;
}

// Retrieve next decimal token from char*. Error values:
//   1 - EOF
//   2 - non-digits
DecimalToken nextDec(const char **cmd) {
    Token t = nextToken(cmd);
    DecimalToken dt = {t, 0, 0};
    if (t.len == 0) {
        dt.err = 1;
        return dt;
    }
    for (unsigned i = 0; i < t.len; i++) {
        if (t.start[i] < '0' || t.start[i] > '9') {
            dt.err = 2;
            return dt;
        }
        dt.val = dt.val * 10 + (t.start[i] - '0');
    }
    return dt;
}

// Print error message relating to a bad parse.
void tokenErr(const char *msg, int start, int len) {
    if (len) {
        int i;
        for (i = 0; i < start; i++) {
            bwputc(COM2, ' ');
        }
        for (i = 0; i < len; i++) {
            bwputc(COM2, '^');
        }
        bwputstr(COM2, "\r\n");
    }
    bwprintf(COM2, "Error: %s\r\n", msg);
}

// Compare token to identifier.
unsigned isIdent(Token t, const char *ident) {
    if (t.len != strlen(ident)) {
        return 0;
    }
    return memcmp(t.start, ident, t.len) == 0;
}

// Print error if there are extra tokens.
unsigned terminateCmd(const char *cmdStart, const char *cmd) {
    Token t = nextToken(&cmd);
    if (t.len) {
        tokenErr("too many arguments", t.start - cmdStart + 2, t.len);
        return 1;
    }
    return 0;
}

// Return 1 if user requested to exit.
int parseCmd(const char *cmd) {
    const char *cmdStart = cmd;
    // Error checking is really messy =(
    Token t = nextToken(&cmd);
    if (t.len == 0) {
        return 0;
    } else if (isIdent(t, "com")) {
        DecimalToken com = nextDec(&cmd);
        if (com.err || (com.val != 1 && com.val != 2)) {
            tokenErr("must be 1 or 2", com.token.start - cmdStart + 2, com.token.len);
            return 0;
        }
        while (1) {
            DecimalToken number = nextDec(&cmd);
            if (number.err == 1) {
                return 0;
            }
            if (number.err || number.val < 0 || number.val > 255) {
                tokenErr("invalid byte", number.token.start - cmdStart + 2, number.token.len);
                return 0;
            }
            bwputc(com.val == 1 ? COM1 : COM2, number.val);
        }
    } else if (isIdent(t, "help")) {
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        printHelp();
    } else if (isIdent(t, "li")) {
        DecimalToken number = nextDec(&cmd);
        if (number.err) {
            tokenErr("invalid train number", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        cmdToggleLight(number.val);
    } else if (isIdent(t, "tr")) {
        DecimalToken number = nextDec(&cmd);
        if (number.err) {
            tokenErr("invalid train number", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        DecimalToken speed = nextDec(&cmd);
        if (speed.err) {
            tokenErr("invalid train speed", speed.token.start - cmdStart + 2, speed.token.len);
            return 0;
        }
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        cmdSetSpeed(number.val, speed.val);
    } else if (isIdent(t, "rv")) {
        DecimalToken number = nextDec(&cmd);
        if (number.err) {
            tokenErr("invalid train number", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        cmdReverse(number.val);
    } else if (isIdent(t, "sw")) {
        DecimalToken number = nextDec(&cmd);
        if (number.err) {
            tokenErr("invalid train number", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        Token dir = nextToken(&cmd);
        if (dir.len == 0) {
            bwputstr(COM2, "Error: expected a direction");
            return 0;
        }
        if (!isIdent(dir, "C") && !isIdent(dir, "S")) {
            tokenErr("invalid direction (must be 'C' or 'S')", dir.start - cmdStart + 2, dir.len);
            return 0;
        }
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        //TODO cmdSetSwitch(number.val, dir.start[0]);
    } else if (isIdent(t, "q")) {
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        return 1;
    } else {
        tokenErr("invalid command name", t.start - cmdStart + 2, t.len);
    }
    return 0;
}
