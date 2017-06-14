// Functions for controlling the trains.

#ifndef TRAIN_H__INCLUDED
#define TRAIN_H__INCLUDED

// Performs the following:
//  - sets the speed of all trains to 0
//  - sends go
void initTrains();

void stopTrains();
void goTrains();

// train: [1, 80]
// time is measured in milliseconds
void updateTrain(int train, unsigned time);

// train: [1, 80]
void cmdToggleLight(int train);

// train: [1, 80]
// speed: [0, 14]
void cmdSetSpeed(int train, int speed);

// train: [1, 80]
void cmdReverse(int train);

#endif // TRAIN_H__INCLUDED
