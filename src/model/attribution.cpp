#include <attribution.h>

Attribution::Attribution(const ModelState &model)
    : model_(model) {
}

Train Attribution::attribute(const Sensor &sensor) {
    // TODO
    (void) sensor;
    return Train(0);
}
