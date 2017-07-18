#include <attribution.h>

#include <cache.h>

Attribution::Attribution(const ModelState &model, const Reservations &reservations)
    : model_(model)
    , reservations_(reservations) {
}

ctl::ErrorOr<Train> Attribution::attribute(const Sensor &sensor) {
    for (auto &t : model_.trains.keys()) {
        if (reservations_.hasReservation(t, sensor.value())) {
            return ctl::ErrorOr<Train>::fromValue(t);
        }
    }
    return ctl::ErrorOr<Train>::fromError(ctl::Error::Unkn);
}
