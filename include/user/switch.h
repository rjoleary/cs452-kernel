#pragma once

// TODO: make stongly typed
typedef int Switch;

constexpr auto NumSwitches = 22;

enum class SwitchState : char {
    Unknown  = 'U',
    DontCare = 'X',
    Straight = 'S',
    Curved   = 'C',
    Neither  = 'N',
};

struct alignas(4) SwitchStates {
    SwitchState states[NumSwitches];
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

    SwitchState & operator[](int sw) {
        return states[toIdx(sw)];
    }

    SwitchState operator[](int sw) const {
        return states[toIdx(sw)];
    }
};

void cmdSetSwitch(Switch sw, SwitchState dir);
void setupSwitches();
void switchMan();

SwitchStates getSwitchData();
SwitchStates waitSwitchChange();
