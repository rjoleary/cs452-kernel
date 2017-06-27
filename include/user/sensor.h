#pragma once

constexpr int NUM_SENSOR_MODULES = 5;
constexpr int NUM_SENSORS_PER_MODULE = 16;

struct alignas(4) SensorSet {
    // One bit array per module.
    unsigned short values[NUM_SENSOR_MODULES] = {0};

    bool operator()(int i, int j) const {
        if (j < 8)
            return values[i] & (1 << (7-j));
        return values[i] & (1 << (15 -j + 8));
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
