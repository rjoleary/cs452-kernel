#include <bwio.h>
#include <sensor.h>
#include <ts7200.h>
#include <ns.h>
#include <io.h>
#include <clock.h>

#define NUM_SENSOR_MODULES 5

void setpos(unsigned, unsigned);
void savecur();
void restorecur();

struct Sensors {
    // One bit array per module.
    unsigned short values[NUM_SENSOR_MODULES] = {0};
};

// TODO: Timeout after 100ms and "reset connection".
void sensorsMain() {
    ~registerAs(ctl::Name{"Sensor"});
    ctl::Tid rx = whoIs(ctl::names::Uart1RxServer).asValue();

    Sensors prevSensors;
    unsigned startOfTriggers = 0;
    for (;;) {
        // Send command
        bwputc(COM1, 0x85);
        flush(COM1);

        // Receive and process bytes.
        Sensors sensors;
        for (int byteId = 0; byteId < 2 * NUM_SENSOR_MODULES; byteId++) {
            char c = io::getc(rx).asValue();
            if (byteId % 2 == 1) {
                sensors.values[byteId/2] |= (
                    (c & (1 << 0)) << 7 |
                    (c & (1 << 1)) << 5 |
                    (c & (1 << 2)) << 3 |
                    (c & (1 << 3)) << 1 |
                    (c & (1 << 4)) >> 1 |
                    (c & (1 << 5)) >> 3 |
                    (c & (1 << 6)) >> 5 |
                    (c & (1 << 7)) >> 7 ) << 8;
            } else {
                sensors.values[byteId/2] =
                    (c & (1 << 0)) << 7 |
                    (c & (1 << 1)) << 5 |
                    (c & (1 << 2)) << 3 |
                    (c & (1 << 3)) << 1 |
                    (c & (1 << 4)) >> 1 |
                    (c & (1 << 5)) >> 3 |
                    (c & (1 << 6)) >> 5 |
                    (c & (1 << 7)) >> 7;
            }
        }

        // Check if any triggers are set before updating.
        bool anyTriggers = false;
        for (int i = 0; i < NUM_SENSOR_MODULES; i++) {
            if ((prevSensors.values[i] ^ sensors.values[i]) != 0) {
                anyTriggers = true;
                break;
            }
        }

        // Print updated sensor info.
        if (anyTriggers) {
            int i, j;
            for (i = 0; i < NUM_SENSOR_MODULES; i++) {
                for (j = 0; j < 16; j++) {
                    if ((prevSensors.values[i] ^ sensors.values[i]) & (1 << j)) {
                        savecur();
                        setpos(4 + startOfTriggers, 3);
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
            setpos(4 + startOfTriggers, 3);
            bwputstr(COM2, "_______");
            restorecur();
            flush(COM2);
            prevSensors = sensors;
        }
    }
}
