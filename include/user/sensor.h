#pragma once

constexpr int NUM_SENSOR_MODULES = 5;
constexpr int NUM_SENSORS_PER_MODULE = 16;

struct alignas(4) Sensors {
    // One bit array per module.
    unsigned short values[NUM_SENSOR_MODULES] = {0};

    bool operator()(int i, int j) const {
        if (j < 8)
            return values[i] & (1 << (7-j));
        return values[i] & (1 << (15 -j + 8));
    }
};

void sensorsMain();

// Return current state of the sensors.
Sensors getSensors();

// Block calling task until any sensor is triggered.
Sensors waitTrigger();
