#include "train_node.h"

Train *new_train(int number, int priority, char direction, int loading_time, int crossing_time){
    Train* newt;
    newt = (Train *)malloc(sizeof(Train));
    if(newt == NULL){
		printf("Error: memory allocation for train node was unsuccessful");
	}
    newt -> priority = priority;
    newt -> direction = direction;
    newt -> loading_time = loading_time;
    newt -> crossing_time = crossing_time;
    newt -> number = number;
    newt -> train_thread = -1;
    newt -> finished_loading = -1;
    newt -> next = NULL;

    return newt;
}