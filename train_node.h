#ifndef TRAIN_H
#define TRAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct Train Train;

struct Train{
    int number; // number of the train within the file

    /* priority will be set to 1 if high and to 0 otherwise*/
    int priority;
    char direction;
    int loading_time;
    int crossing_time;
    pthread_t train_thread;
    pthread_cond_t terminating_cond;
    double finished_loading;
    Train *next;
};

extern Train *new_train(int, int, char, int, int);

#endif