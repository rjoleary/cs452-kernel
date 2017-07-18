#include <attribution.h>

#include <cache.h>

Attribution::Attribution(const ModelState &model, const Reservations &reservations)
    : model_(model)
    , reservations_(reservations) {
}

ctl::ErrorOr<Train> Attribution::attribute(const Sensor &sensor) {
    for (Size idx; idx < model_.trains.size(); idx++) {
        Train t = model_.trains.getKey(idx);
        if (reservations_.hasReservation(t, sensor.value())) {
            return ctl::ErrorOr<Train>::fromValue(t);
        }
    }
    return ctl::ErrorOr<Train>::fromError(ctl::Error::Unkn);
}
