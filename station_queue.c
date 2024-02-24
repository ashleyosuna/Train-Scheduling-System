#include "station_queue.h"

Train *enqueue_train(Train *queue, Train *new_train){
    /* Function that takes a queue and a train struct and places it
       in the given queue based on priority (i.e., trains are placed after
       last train with the same priority)*/
    if(queue == NULL){
        return new_train;
    }
    Train *prev = NULL;
    Train *curr;

    /* updating pointer to the correct location within the queue */
    for(curr = queue; curr != NULL && curr ->priority >= new_train -> priority 
        && curr -> finished_loading <= new_train -> finished_loading; curr = curr -> next){
            
            /* if two trains finished loading at the same time, break if new_train 
               appeared before curr in the file */
            if(curr -> finished_loading == new_train -> finished_loading &&
               curr -> number > new_train -> number){
                break;
            }
            prev = curr;
    }

    if(prev != NULL){
        prev -> next = new_train;
    }
    new_train -> next = curr;
    if(prev == NULL){
        queue = new_train;
    }
    return queue;
}

Train *dequeue_train(Train *queue){
    /* Function that takes a queue and removes Train struct at the
       front of the queue */
    if(queue == NULL){ // nothing to dequeue
        return NULL;
    }

    queue = queue -> next;
    return queue;
}