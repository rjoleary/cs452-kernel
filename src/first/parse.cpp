#include <bwio.h>
#include <train.h>
#include <std.h>
#include <switch.h>
#include <sensor.h>
#include <initializer_list>
#include <track.h>

#include <task.h>
#include <ns.h>
#include <err.h>
#include <std.h>
#include <def.h>
#include <safety.h>
#include <route.h>
#include <callibration.h>

void printHelp() {
    bwputstr(COM2,
        "Commands:\r\n"
        "   cal TR SP SEN      - stop train on the next trigger.\r\n"
        "   com i [BYTE...]    - Send arbitrary byte over COMi.\r\n"
        "   help               - Display this help information.\r\n"
        "   q                  - Quit and return to RedBoot.\r\n"
        "   route TR SP SN [D] - Route train to sensor at given speed.\r\n"
        "   rv NUMBER          - Reverse the direction of the train.\r\n"
        "   ssd TR SP NUMBER   - Set the stopping distance in mm.\r\n"
        "   sw NUM DIR [TR]    - Set switch direction ('S' or 'C').\r\n"
        "   task (TID|NAME)    - Return info about a task.\r\n"
        "   taskall            - Return info about all tasks.\r\n"
        "   tr NUMBER SPEED    - Set train speed (0 for stop).\r\n"
        "   unbreak            - Mark all sensors as unbroken.\r\n"
    );
}

struct Token {
    const char *start;
    unsigned len;
};

struct DecimalToken {
    Token token;
    int val;
    unsigned err; // 0 - success, 1 - error
};

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

    // TODO: Make the parser a class too!
    static TrainServer trainServer;
    static SafetyServer safety;
    static RouteServer route;

    const char *cmdStart = cmd;
    // Error checking is really messy =(
    Token t = nextToken(&cmd);
    if (t.len == 0) {
        return 0;
    } else if (isIdent(t, "cal")) {
        DecimalToken number = nextDec(&cmd);
        if (number.err || number.val < 1 || number.val > 80) {
            tokenErr("invalid train number", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        DecimalToken speed = nextDec(&cmd);
        if (speed.err || speed.val < 0 || speed.val > 14) {
            tokenErr("invalid train speed", speed.token.start - cmdStart + 2, speed.token.len);
            return 0;
        }
        Token sensor = nextToken(&cmd);
        if (sensor.len == 0) {
            bwputstr(COM2, "Error: expected a direction\r\n");
            return 0;
        }
        auto sensorParsed = Sensor::fromString(sensor.start, sensor.len);
        if (sensorParsed.isError()) {
            tokenErr("invalid sensor", sensor.start - cmdStart + 2, sensor.len);
            return 0;
        }
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        ctl::Error err = safety.calibrate(Train(number.val), sensorParsed.asValue(), speed.val);
        if (err != ctl::Error::Ok) {
            if (err == ctl::Error::NoRes) {
                bwputstr(COM2, "Error: too many concurrent trains\r\n");
            } else {
                bwprintf(COM2, "Error: %s\r\n", errorToString(err));
            }
        }
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
        if (number.val < 1 || 80 < number.val) {
            bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
            return 0;
        }
        if (speed.val < 0 || 14 < speed.val) {
            bwputstr(COM2, "Error: speed must be between 0 and 14 inclusive\r\n");
            return 0;
        }
        ctl::Error err = safety.setTrainSpeed(Train(number.val), speed.val);
        if (err != ctl::Error::Ok) {
            if (err == ctl::Error::NoRes) {
                bwputstr(COM2, "Error: too many concurrent trains\r\n");
            } else {
                bwprintf(COM2, "Error: %s\r\n", errorToString(err));
            }
        }
    } else if (isIdent(t, "route")) {
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
        Token sensor = nextToken(&cmd);
        if (sensor.len == 0) {
            bwputstr(COM2, "Error: expected a sensor\r\n");
            return 0;
        }
        auto sensorParsed = Sensor::fromString(sensor.start, sensor.len);
        if (sensorParsed.isError()) {
            tokenErr("invalid sensor", sensor.start - cmdStart + 2, sensor.len);
            return 0;
        }
        Distance offset = 0;
        DecimalToken offsetToken = nextDec(&cmd);
        if (offsetToken.err == 2) {
            tokenErr("offset must be numeric", offsetToken.token.start - cmdStart + 2, offsetToken.token.len);
            return 0;
        }
        if (offsetToken.err == 0) {
            offset = offsetToken.val;
        }
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        if (number.val < 1 || 80 < number.val) {
            tokenErr("train number must be between 1 and 80 inclusive", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        if (speed.val < 0 || 14 < speed.val) {
            tokenErr("Error: speed must be between 0 and 14 inclusive", speed.token.start - cmdStart + 2, speed.token.len);
            return 0;
        }
        route.update(Train(number.val), speed.val, Position{sensorParsed.asValue().value(), offset});
    } else if (isIdent(t, "rv")) {
        DecimalToken number = nextDec(&cmd);
        if (number.err) {
            tokenErr("invalid train number", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        if (number.val < 1 || 80 < number.val) {
            bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
            return 0;
        }
        safety.reverseTrain(Train(number.val));
    } else if (isIdent(t, "sw")) {
        DecimalToken number = nextDec(&cmd);
        if (number.err) {
            tokenErr("invalid switch number", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        Token dir = nextToken(&cmd);
        if (dir.len == 0) {
            bwputstr(COM2, "Error: expected a direction\r\n");
            return 0;
        }
        if (!isIdent(dir, "C") && !isIdent(dir, "S")) {
            tokenErr("invalid direction (must be 'C' or 'S')", dir.start - cmdStart + 2, dir.len);
            return 0;
        }
        DecimalToken train = nextDec(&cmd);
        if (train.err) {
            if (terminateCmd(cmdStart, cmd)) {
                return 0;
            }
            cmdSetSwitch(number.val, SwitchState(dir.start[0]));
        } else {
            if (train.val < 1 || 80 < train.val) {
                tokenErr("train number must be between 1 and 80 inclusive", train.token.start - cmdStart + 2, train.token.len);
                return 0;
            }
            safety.setSwitch(Train(train.val), number.val, SwitchState(dir.start[0]));
        }
    } else if (isIdent(t, "task")) {
        ctl::Tid tid;
        DecimalToken number = nextDec(&cmd);
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        if (number.err == 1) {
            tokenErr("expected an argument", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        } else if (number.err == 0) {
            tid = ctl::Tid(number.val);
        } else {
            ctl::Name name;
            if (number.token.len > sizeof(name.data) - 1) {
                tokenErr("task name too long", number.token.start - cmdStart + 2, number.token.len);
                return 0;
            }
            memset(name.data, 0, sizeof(name.data));
            memcpy(name.data, number.token.start, number.token.len);
            auto resp = whoIs(name);
            if (resp.isError()) {
                tokenErr("unknown task name", number.token.start - cmdStart + 2, number.token.len);
                return 0;
            }
            tid = resp.asValue();
        }
        ctl::TaskInfo ti;
        if (ctl::taskInfo(tid, &ti) != ctl::Error::Ok) {
            tokenErr("invalid task id", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        ctl::Name name{""};
        reverseWhoIs(tid, &name);
        bwputstr(COM2, "TID\tNAME\tPTID\tPRI\tSTATE\tUSER\tSYS\r\n");
        bwprintf(COM2, "%d\t%s\t%d\t%d\t%c\t%d%%\t%d%%\r\n",
            ti.tid, name.data, ti.ptid, ti.pri, ti.state, ti.userPercent, ti.sysPercent);
    } else if (isIdent(t, "taskall")) {
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        bwputstr(COM2, "TID\tNAME\tPTID\tPRI\tSTATE\tUSER\tSYS\r\n");
        for (int tid = 0; tid < NUM_TD; tid++) {
            ctl::TaskInfo ti;
            if (ctl::taskInfo(ctl::Tid(tid), &ti) == ctl::Error::Ok) {
                ctl::Name name{""};
                reverseWhoIs(ctl::Tid(tid), &name);
                bwprintf(COM2, "%d\t%s\t%d\t%d\t%c\t%d%%\t%d%%\r\n",
                    ti.tid, name.data, ti.ptid, ti.pri, ti.state, ti.userPercent, ti.sysPercent);
            }
        }
    } else if (isIdent(t, "ssd")) {
        DecimalToken number = nextDec(&cmd);
        if (number.err || number.val < 0 || number.val > 80) {
            tokenErr("invalid train number", number.token.start - cmdStart + 2, number.token.len);
            return 0;
        }
        DecimalToken speed = nextDec(&cmd);
        if (speed.err || speed.val < 0 || speed.val > 14) {
            tokenErr("invalid train speed", speed.token.start - cmdStart + 2, speed.token.len);
            return 0;
        }
        DecimalToken millis = nextDec(&cmd);
        if (millis.err) {
            tokenErr("invalid millimeters", millis.token.start - cmdStart + 2, millis.token.len);
            return 0;
        }
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        CallibrationServer().setStoppingDistance(Train(number.val), speed.val, millis.val);
    } else if (isIdent(t, "unbreak")) {
        if (terminateCmd(cmdStart, cmd)) {
            return 0;
        }
        cmdClearBrokenSwitches();
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
