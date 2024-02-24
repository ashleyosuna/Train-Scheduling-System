#ifndef STATION_QUEUE_H
#define STATION_QUEUE_H

#include "train_node.h"

extern Train *enqueue_train(Train *, Train *);
extern Train *dequeue_train(Train *);

#endif