#pragma once

// TODO: make stongly typed
typedef int Switch;

void cmdSetSwitch(int sw, char dir);
void setupSwitches();
void switchMan();

constexpr auto NumSwitches = 22;

struct alignas(4) SwitchStates {
    char states[NumSwitches];
    static inline int toIdx(int sw) {
        if (1 <= sw && sw <= 18)
            return sw - 1;
        else
            return sw - 153 + 18;
    }
    static inline int fromIdx(int sw) {
        if (0 <= sw && sw < 18)
            return sw + 1;
        else
            return sw + 153 - 18;
    }

    char & operator[](int sw) {
        return states[toIdx(sw)];
    }

    const char & operator[](int sw) const {
        return states[toIdx(sw)];
    }
};

SwitchStates getSwitchData();
SwitchStates waitSwitchChange();
