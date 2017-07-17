#pragma once

#include "model.h"
#include "sensor.h"
#include "train.h"

class Attribution {
    const ModelState &model_;

  public:
    Attribution(const ModelState &model);
    Train attribute(const Sensor &sensor);
};
