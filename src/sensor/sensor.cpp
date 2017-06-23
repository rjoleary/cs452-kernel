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
        delay(clock, 1);
        send(server, msg, ctl::EmptyMessage);
    }
}

void printUpdate(const Sensors &prevSensors, const Sensors &sensors, unsigned &startOfTriggers) {
    for (int i = 0; i < NUM_SENSOR_MODULES; i++) {
        for (int j = 0; j < 16; j++) {
            if ((prevSensors.values[i] ^ sensors.values[i]) & (1 << j)) {
                savecur();
                setpos(4 + startOfTriggers, 40);
                startOfTriggers = (startOfTriggers + 1) % 11;
                bwprintf(COM2, "%c%d ", 'a' + i, j + 1);
                if (sensors.values[i] & (1 << j)) {
                    bwputstr(COM2, "ON   ");
                } else {
                    bwputstr(COM2, "OFF  ");
                }
                restorecur();
                flush(COM2);
            }
        }
    }
    savecur();
    setpos(4 + startOfTriggers, 40);
    bwputstr(COM2, "_______");
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
        while (dataRead < 10 && timeoutsRecv < 11) {
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
        if (dataRead < 10) continue;

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
