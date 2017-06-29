#pragma once

#include <bwio.h>
#include <std.h>

using namespace ctl;

constexpr int NUM_SENSOR_MODULES = 5;
constexpr int NUM_SENSORS_PER_MODULE = 16;

class Sensor {
    unsigned char module_; // [0-4]
    unsigned char sensor_; // [0-15]

    static bool isModule(char c) {
        return ('a' <= c && c <= 'e') || ('A' <= c && c <= 'E');
    }

    static bool isDigit(char c) {
        return '0' <= c && c <= '9';
    }

public:
    // Range: [0-79]
    // TODO: use ErrorOr
    static Sensor fromInt(int value, bool &error) {
        Sensor s;
        if (value < 0 || 79 < value) {
            error = true;
            return s;
        }
        s.module_ = value / 16;
        s.sensor_ = value % 16;
        error = false;
        return s;
    }

    // Must be: [A-Ea-e]{1-16}
    // TODO: user ErrorOr
    static Sensor fromString(const char *str, size_t len, bool &error) {
        Sensor s;
        if (len != 2 && len != 3) {
            error = true;
            return s;
        }
        if (!isModule(str[0]) || !isDigit(str[1])) {
            error = true;
            return s;
        }
        s.module_ = (str[0] | 0x20) - 'a';
        s.sensor_ = str[1] - '0';
        if (len == 3) {
            if (!isDigit(str[2])) {
                error = true;
                return s;
            }
            s.sensor_ = s.sensor_ * 10 + (str[2] - '0');
        }
        if (s.sensor_ < 1 || 16 < s.sensor_) {
            error = true;
            return s;
        }
        s.sensor_--;
        error = false;
        return s;
    }

    unsigned char module() const {
        return module_;
    }

    unsigned char sensor() const {
        return sensor_;
    }

    unsigned char value() const {
        return module_ * 16 + sensor_;
    }

    void print(int com) const {
        bwprintf(com, "%c%d", module_ + 'A', sensor_);
    }
};

struct alignas(4) SensorSet {
    // One bit array per module.
    unsigned short values[NUM_SENSOR_MODULES] = {0};

    bool operator()(int i, int j) const {
        if (j < 8)
            return values[i] & (1 << (7-j));
        return values[i] & (1 << (15 -j + 8));
    }

    bool operator()(Sensor s) const {
        return (*this)(s.module(), s.sensor());
    }

    operator bool() const {
        for (const auto &x : values) {
            if (x) {
                return true;
            }
        }
        return false;
    }

    SensorSet& operator^=(const SensorSet &other) {
        for (int i = 0; i < NUM_SENSOR_MODULES; i++) {
            values[i] ^= other.values[i];
        }
        return *this;
    }

    SensorSet operator^(const SensorSet &other) const {
        SensorSet ret = *this;
        ret ^= other;
        return ret;
    }
};

void sensorsMain();

// Return current state of the sensors.
SensorSet getSensors();

// Block calling task until any sensor is triggered.
SensorSet waitTrigger();

void markBrokenSensor(int sensor);
