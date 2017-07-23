#pragma once

#include "err.h"
#include "safety.h"
#include "reservations.h"
#include "sensor.h"
#include "train.h"

class Attribution {
    const SafetyState &safety_;
    const Reservations &reservations_;

  public:
    Attribution(const SafetyState &safety, const Reservations &reservations_);
    ctl::ErrorOr<Train> attribute(const Sensor &sensor);
};
