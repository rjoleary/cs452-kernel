#include <attribution.h>

Attribution::Attribution(const SafetyState &safety, const Reservations &reservations)
    : safety_(safety)
    , reservations_(reservations) {
}

ctl::ErrorOr<Train> Attribution::attribute(const Sensor &sensor) {
    for (const auto &t : safety_.trains.keys()) {
        if (reservations_.hasReservation(t, sensor.value())) {
            return ctl::ErrorOr<Train>::fromValue(t);
        }
    }
    return ctl::ErrorOr<Train>::fromError(ctl::Error::Unkn);
}
