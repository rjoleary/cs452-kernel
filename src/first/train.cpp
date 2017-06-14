#include <bwio.h>
#include <train.h>

typedef enum {
    // Train has unknown lights and speed. Assume trains are all set to speed 0
    // and transition to StateRunning.
    StateUnknown = 0,

    // Stay running at the same speed.
    StateRunning,

    // Set the train's speed.
    StateSetSpeed,

    // Slow down to reverse. Once the stop is reversed, transition to
    // StateRunning.
    StateSlowToReverse,
    StateSlowToReverseWait,
    StateReverse,
} State;

typedef struct {
    //State state;
    char state;
    // light and speed
    char speed;
    // Estimate time when the train will come to a stop.
    unsigned stopTime;
} Train;

// Train #1 is at index 0.
static Train trains[80];

void initTrains() {
    // Set train speeds to 0.
    int i;
    for (i = 0; i < 80; i++) {
        trains[i].state = StateUnknown;
        trains[i].speed = 0;
    }

    goTrains();
}

/*void updateTrain(int train, unsigned time) {
    Train *t = &trains[train-1];
    switch (t->state) {

    case StateUnknown:
        t->state = StateRunning;
        break;

    case StateRunning:
        break;

    case StateSetSpeed:
        bufferEnqueue2(COM1, t->speed, train);
        t->state = StateRunning;
        break;

    case StateSlowToReverse:
        if (t->speed == 0) {
            t->state = StateReverse;
        } else {
            bufferEnqueue2(COM1, t->speed & ~0x0f, train);
            // 200 milliseconds for each train speed
            t->stopTime = time + t->speed * 200;
            t->state = StateSlowToReverseWait;
        }
        break;

    case StateSlowToReverseWait:
        if (time >= t->stopTime) {
            bufferEnqueue2(COM1, t->speed | 0x0f, train);
            t->state = StateReverse;
        }
        break;

    case StateReverse:
        if (time >= t->stopTime + 150) {
            t->state = StateSetSpeed;
        }
        break;
    }
}*/

void stopTrains() {
    bwputc(COM1, 97);
}

void goTrains() {
    bwputc(COM1, 96);
}

void cmdToggleLight(int train) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
    }
    Train *t = &trains[train-1];
    t->speed ^= 0x10; // toggle light
    bwputc(COM1, t->speed);
    bwputc(COM1, train);
}

void cmdSetSpeed(int train, int speed) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
    }
    if (speed < 0 || 14 < speed) {
        bwputstr(COM2, "Error: speed must be between 0 and 15 inclusive\r\n");
        return;
    }
    Train *t = &trains[train-1];
    t->speed = (t->speed & ~0x0f) | speed;
    t->state = StateSetSpeed;
}

void cmdReverse(int train) {
    if (train < 1 || 80 < train) {
        bwputstr(COM2, "Error: train number must be between 1 and 80 inclusive\r\n");
    }
    trains[train-1].state = StateSlowToReverse;
}
