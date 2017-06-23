#pragma once

constexpr int NUM_SENSOR_MODULES = 5;

struct alignas(4) Sensors {
    // One bit array per module.
    unsigned short values[NUM_SENSOR_MODULES] = {0};
};

void sensorsMain();

// Return current state of the sensors.
void getSensors(Sensors *sensors);

// Block calling task until any sensor is triggered.
void waitTrigger(Sensors *sensors);
