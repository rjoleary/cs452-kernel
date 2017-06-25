#include <def.h>
#include <bwio.h>
#include <sensor.h>
#include <ts7200.h>
#include <ns.h>
#include <io.h>
#include <clock.h>
#include <itc.h>
#include <circularbuffer.h>

// TODO: include declarations from header file
void setpos(unsigned, unsigned);
void savecur();
void restorecur();

extern const char *LAYOUT;

namespace {
enum class MsgType {
    Recv,
    Timer,
    GetSensors,
    WaitTrigger,
};

struct Message {
    MsgType type;
    char data;
};

constexpr ctl::Name SensorServ = {"Sensor"};

void recvNotif() {
    auto server = whoIs(SensorServ).asValue();
    auto rx = whoIs(ctl::names::Uart1RxServer).asValue();
    Message msg {MsgType::Recv};
    for (;;) {
        char c = io::getc(rx).asValue();
        msg.data = c;
        send(server, msg, ctl::EmptyMessage);
    }
}

void timeoutNotif() {
    auto server = whoIs(SensorServ).asValue();
    auto clock = whoIs(ctl::names::ClockServer).asValue();
    Message msg {MsgType::Timer};
    for (;;) {
        delay(clock, 2);
        send(server, msg, ctl::EmptyMessage);
    }
}

const struct {
    char row, col;
    char character;
} LAYOUT_POS[] = {
    // A1-A16
    {1, 5, '<'},
    {1, 5, '>'},
    {5, 7, '^'},
    {5, 7, 'v'},
    {3, 19, '>'},
    {3, 19, '<'},
    {13, 5, '<'},
    {13, 5, '>'},
    {11, 3, '<'},
    {11, 3, '>'},
    {9, 3, '>'},
    {9, 3, '<'},
    {3, 3, '>'},
    {3, 3, '<'},
    {5, 3, '<'},
    {5, 3, '>'},

    // B1-B16
    {11, 19, '>'},
    {11, 19, '<'},
    {10, 18, '^'},
    {10, 18, 'v'},
    {3,  19, '>'},
    {3,  19, '<'},
    {11, 2,  '>'},
    {11, 2,  '<'},
    {15, 2,  '>'},
    {15, 2,  '<'},
    {13, 2,  '>'},
    {13, 2,  '<'},
    {9,  21, '^'},
    {9,  21, 'v'},
    {9,   7, '^'},
    {9,   7, 'v'},

    // C1-C16
    {9, 19, 'v'},
    {9, 19, '^'},
    {15, 26, '>'},
    {15, 26, '<'},
    {13, 12, '>'},
    {13, 12, '<'},
    {15, 13, '>'},
    {15, 13, '<'},
    {9,  10, '<'},
    {9,  10, '>'},
    {3,  12, '>'},
    {3,  12, '<'},
    {1,  14, '<'},
    {1,  14, '>'},
    {13, 16, '>'},
    {13, 16, '<'},

    // D1-D16
    {1, 5, '<'},
    {1, 5, '>'},
    {5, 7, '^'},
    {5, 7, 'v'},
    {3, 19, '>'},
    {3, 19, '<'},
    {13, 5, '<'},
    {13, 5, '>'},
    {11, 3, '<'},
    {11, 3, '>'},
    {9, 3, '>'},
    {9, 3, '<'},
    {3, 3, '>'},
    {3, 3, '<'},
    {5, 3, '<'},
    {5, 3, '>'},

    // E1-E16
    {1, 5, '<'},
    {1, 5, '>'},
    {5, 7, '^'},
    {5, 7, 'v'},
    {3, 19, '>'},
    {3, 19, '<'},
    {13, 5, '<'},
    {13, 5, '>'},
    {11, 3, '<'},
    {11, 3, '>'},
    {9, 3, '>'},
    {9, 3, '<'},
    {3, 3, '>'},
    {3, 3, '<'},
    {5, 3, '<'},
    {5, 3, '>'},

    // F1-F16
    {1, 5, '<'},
    {1, 5, '>'},
    {5, 7, '^'},
    {5, 7, 'v'},
    {3, 19, '>'},
    {3, 19, '<'},
    {13, 5, '<'},
    {13, 5, '>'},
    {11, 3, '<'},
    {11, 3, '>'},
    {9, 3, '>'},
    {9, 3, '<'},
    {3, 3, '>'},
    {3, 3, '<'},
    {5, 3, '<'},
    {5, 3, '>'},
};

void printUpdate(const Sensors &prevSensors, const Sensors &sensors, unsigned &startOfTriggers) {
    savecur();
    for (int i = 0; i < NUM_SENSOR_MODULES; i++) {
        for (int j = 0; j < 16; j++) {
            if ((prevSensors.values[i] ^ sensors.values[i]) & (1 << j)) {
                setpos(4 + startOfTriggers, 40);
                startOfTriggers = (startOfTriggers + 1) % 11;
                bwprintf(COM2, "%c%d ", 'a' + i, j + 1);
                if (sensors.values[i] & (1 << j)) {
                    bwputstr(COM2, "ON   ");
                } else {
                    bwputstr(COM2, "OFF  ");
                }
                flush(COM2);
            }
        }
    }
    setpos(4 + startOfTriggers, 40);
    bwputstr(COM2, "_______");
    flush(COM2);

    for (int i = 0; i < NUM_SENSOR_MODULES; i++) {
        for (int j = 0; j < 16; j++) {
            if ((prevSensors.values[i] ^ sensors.values[i]) & (1 << j)) {
                auto layout = LAYOUT_POS[i*16+j];
                setpos(2 + layout.row, layout.col);
                if (sensors.values[i] & (1 << j)) {
                    bwputstr(COM2, "\033[36m");
                    bwputc(COM2, layout.character);
                    bwputstr(COM2, "\033[0m");
                } else {
                    bwputc(COM2, LAYOUT[(layout.row+1)*36+layout.col-1]);
                }
            }
        }
    }
    restorecur();
    flush(COM2);
}
}

void sensorsMain() {
    ~registerAs(SensorServ);

    create(ctl::Priority(30), recvNotif);
    create(ctl::Priority(30), timeoutNotif);

    Sensors prevSensors;
    unsigned startOfTriggers = 0;

    ctl::CircularBuffer<ctl::Tid, NUM_TD> triggerBlocked;

    for (;;) {
        int dataRead = 0, timeoutsRecv = 0;
        Sensors sensors;
        // Send command
        bwputc(COM1, 0x85);
        flush(COM1);
        while (dataRead < 10 && timeoutsRecv < 6) {
            ctl::Tid tid;
            Message msg;
            ~receive(&tid, msg);
            switch (msg.type) {
                case MsgType::Recv: {
                    ~reply(tid, ctl::EmptyMessage);
                    auto c = msg.data;
                    if (dataRead % 2 == 1) {
                        sensors.values[dataRead/2] |= (
                                (c & (1 << 0)) << 7 |
                                (c & (1 << 1)) << 5 |
                                (c & (1 << 2)) << 3 |
                                (c & (1 << 3)) << 1 |
                                (c & (1 << 4)) >> 1 |
                                (c & (1 << 5)) >> 3 |
                                (c & (1 << 6)) >> 5 |
                                (c & (1 << 7)) >> 7 ) << 8;
                    } else {
                        sensors.values[dataRead/2] =
                            (c & (1 << 0)) << 7 |
                            (c & (1 << 1)) << 5 |
                            (c & (1 << 2)) << 3 |
                            (c & (1 << 3)) << 1 |
                            (c & (1 << 4)) >> 1 |
                            (c & (1 << 5)) >> 3 |
                            (c & (1 << 6)) >> 5 |
                            (c & (1 << 7)) >> 7;
                    }
                    dataRead++;
                    timeoutsRecv = 0;
                    break;
                }

                case MsgType::Timer: {
                    ~reply(tid, ctl::EmptyMessage);
                    timeoutsRecv++;
                    break;
                }

                case MsgType::GetSensors: {
                    ~reply(tid, prevSensors);
                    break;
                }

                case MsgType::WaitTrigger: {
                    triggerBlocked.push(tid);
                    break;
                }
            }
        }
        if (dataRead < 10) {
            bwprintf(COM2, "Restarted sensor reading\r\n");
            continue;
        }

        for (int i = 0; i < NUM_SENSOR_MODULES; i++) {
            if ((prevSensors.values[i] ^ sensors.values[i]) != 0) {
                // Print sensors
                printUpdate(prevSensors, sensors, startOfTriggers);

                // Trigger tasks
                while (!triggerBlocked.empty()) {
                    ~reply(triggerBlocked.pop(), sensors);
                }

                prevSensors = sensors;
                break;
            }
        }
    }
}

void getSensors(Sensors *sensors) {
    ~send(whoIs(SensorServ).asValue(), Message{MsgType::GetSensors}, *sensors);
}

void waitTrigger(Sensors *sensors) {
    ~send(whoIs(SensorServ).asValue(), Message{MsgType::WaitTrigger}, *sensors);
}
