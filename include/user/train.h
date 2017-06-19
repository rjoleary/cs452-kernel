// Functions for controlling the trains.
#pragma once

void stopTrains();
void goTrains();

// train: [1, 80]
void cmdToggleLight(int train);

// train: [1, 80]
// speed: [0, 14]
void cmdSetSpeed(int train, int speed);

// train: [1, 80]
void cmdReverse(int train);
