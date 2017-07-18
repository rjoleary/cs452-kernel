#pragma once

#include "err.h"
#include "model.h"
#include "reservations.h"
#include "sensor.h"
#include "train.h"

class Attribution {
    const ModelState &model_;
    const Reservations &reservations_;

  public:
    Attribution(const ModelState &model, const Reservations &reservations_);
    ctl::ErrorOr<Train> attribute(const Sensor &sensor);
};
