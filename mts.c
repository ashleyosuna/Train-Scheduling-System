#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "train_node.h"
#include "station_queue.h"

#define BILLION 1000000000L;

pthread_mutex_t schedule, printing;
pthread_cond_t start_loading, job_available, threads_created;
int num_threads  = 0;
int lines_read   = 0;
struct timespec start;
Train *east_station   = NULL;
Train *west_station   = NULL;

void check_cond_init(int status){
    /* checks if conditions were initializes successfully based
       on return value of pthread_cond_init() function */
    if(status != 0){
        perror("pthread_cond_init() error");
        exit(1);
    }
}

void check_locks(int status){
    /* function for checking that pthread_mutex_lock() returned
       succesfully */
    if(status != 0){
        perror("pthread_mutex_lock() error");
        exit(1);
    }
}

void check_cond_wait(int status){
    /* function for checking that pthread_cond_wait() returned
       succesfully */
    if(status != 0){
        perror("pthread_cond_wait() error");
        exit(1);
    }
}

void check_signal(int status){
    /* function for checking that pthread_cond_signal() returned
       succesfully */
    if(status != 0){
        perror("pthread_cond_signal() error");
        exit(1);
    }
}

void check_unlocks(int status){
    /* function for checking that pthread_mutex_unlock() returned
       succesfully */
    if(status != 0){
        perror("pthread_mutex_unlock() error");
        exit(1);
    }
}

void check_join(int status){
    /* function for checking that pthread_join() returned
       succesfully */
    if(status != 0){
        perror("pthread_join() error");
        exit(1);
    }
}

void check_mutex_destroy(int status){
    /* function for checking that pthread_mutex_destroy() returned
       succesfully */
    if(status != 0){
        perror("pthread_mutex_destroy() error");
        exit(1);
    }
}

void check_cond_destroy(int status){
    /* function for checking that pthread_cond_destroy() returned
       succesfully */
    if(status != 0){
        perror("pthread_cond_destroy() error");
        exit(1);
    }
}

float print_time(){
    /* function that prints current time relative to the start
       of the program */
    struct timespec curr_time;
    float accum;

    /* used example from tutorial for implementation of the clock */
    if(clock_gettime(CLOCK_REALTIME, &curr_time) == -1){
        perror("clock gettime error");
        exit(1);
    }

    accum = (curr_time.tv_sec - start.tv_sec) + 
            (curr_time.tv_nsec - start.tv_nsec) / (double)BILLION;

    int hours = (int)(accum / 3600);
    int minutes = (int)((accum - (hours * 3600.0)) / 60);
    float seconds = (float)(accum - (hours * 3600.0) - (minutes * 60.0));
    printf("%02d:%02d:%04.1f ", hours, minutes, seconds);
    return accum;
}

void load_train(Train *train){
    /* function that simulates train loading by sleeping for the time
       given as loading time for the train; function then prints
       when the train has finished loading */
    int sleep = usleep((useconds_t)(train -> loading_time*100000));
    if(sleep != 0){
        perror("usleep() error");
        exit(1);
    }
    int status = pthread_mutex_lock(&printing);
    check_locks(status);
    float finished_loading = print_time();

    /* round down to get expected behavior when two trains with
       same priority are at the same station (i.e., schedule train
       that first appeared in the file first) */
    float round_down = floorf(finished_loading * 10)/100;
    train -> finished_loading = round_down;
    printf("Train %d is ready to go ", train-> number);
    if(train -> direction == 'e'){
        printf("East\n");
    }else{
        printf("West\n");
    }
    status = pthread_mutex_unlock(&printing);
    check_unlocks(status);
    return;
}

void *init_train(void *passed_train){
    /* Function takes the information of each train and initializes corresponding
       Train node to store said information */

    Train *train = (Train *)passed_train;
    int init = pthread_cond_init(&train -> terminating_cond, NULL);
    check_cond_init(init);

    /* acquiring lock */
    int lock = pthread_mutex_lock(&schedule);
    check_locks(lock);

    num_threads++;
    if(num_threads == lines_read){
        /* signaling to main thread that all train threads have been created */
        int status = pthread_cond_signal(&threads_created);
        check_signal(status);
    }

    /* train waiting to be signaled to start loading */
    int cond = pthread_cond_wait(&start_loading, &schedule);
    check_cond_wait(cond);

    int status = pthread_mutex_unlock(&schedule);
    check_unlocks(status);
    
    load_train(train);

    pthread_mutex_lock(&schedule);

    /* trains placing themselves into corresponding station queues */
    if(train -> direction == 'e'){
        west_station = enqueue_train(west_station, train);
    }else{
        east_station = enqueue_train(east_station, train);
    }
    status = pthread_cond_signal(&job_available);
    check_signal(status);

    /* waiting for main thread to indicate that thread should terminate
       (i.e., disappear) */
    status = pthread_cond_wait(&train -> terminating_cond, &schedule);
    check_cond_wait(status);
    
    status = pthread_mutex_unlock(&schedule);
    check_unlocks(status);

    status = pthread_cond_destroy(&train -> terminating_cond);
    check_cond_destroy(status);
    pthread_exit(NULL);
}

void cross_track(Train *crossing_train, char *direction){
    /* function that simulates crossing of scheduled train by sleeping
       for time specified by the train's crossing time; function prints
       time when train is on the main track and time when it finishes crossing*/
    int status = pthread_mutex_lock(&printing);
    check_locks(status);
    print_time();
    printf("Train %d is ON the main track going %s\n", crossing_train -> number, direction);
    status = pthread_mutex_unlock(&printing);
    check_unlocks(status);
        
    /* train is crossing */
    int sleep = usleep((useconds_t)(crossing_train -> crossing_time * 100000));
    if(sleep != 0){
        perror("usleep() error");
        exit(1);
    }
        
    /* print time after train finished crossing */
    status = pthread_mutex_lock(&printing);
    check_locks(status);
    print_time();
    printf("Train %d is OFF the main track after going %s\n", crossing_train -> number, direction);
    status = pthread_mutex_unlock(&printing);
    check_unlocks(status);
    return;
}

void schedule_and_cross(){
    Train *crossing_train;
    char last_dir = 'e'; // initialized to 'e' to give priority to westbound
                         // train if trains with same priorities are at both stations
    int consecutive_trains = 1;
    int status;
    
    while(1){
        status = pthread_mutex_lock(&schedule);
        check_locks(status);
        if(num_threads == 0){
            /* breaking if all trains have crossed */
            status = pthread_mutex_unlock(&schedule);
            check_unlocks(status);
            break;
        }

        if(west_station == NULL && east_station == NULL){
            /* if both queues are empty, wait until a job is available */
            status = pthread_cond_wait(&job_available, &schedule);
            check_cond_wait(status);
        } 
        
        num_threads--;
        if(west_station == NULL){
            /* scheduling train at east_station by default */
            crossing_train = east_station;
            east_station = dequeue_train(east_station);

        }else if(east_station == NULL){
            /* scheduling train at west_station by default */
            crossing_train = west_station;
            west_station = dequeue_train(west_station);

        }else if(consecutive_trains >= 3 || 
            west_station -> priority == east_station -> priority ){
            /* if three trains have crossed in the same direction consecutively
               dispatch a train going in the opposite direction to avoid starvation.
               if priorities at the head of both stations, alternate between them*/
            if(last_dir == 'e'){
                crossing_train = east_station;
                east_station = dequeue_train(east_station);
            }else{
                crossing_train = west_station;
                west_station = dequeue_train(west_station);
            }

        }else if(west_station -> priority > east_station -> priority){
            /* selecting train at the front of west station if it has 
               higher priority */
            crossing_train = west_station;
            west_station = dequeue_train(west_station);

        }else if(east_station -> priority > west_station -> priority){
            /* selecting train at the front of east station if it has 
               higher priority */
            crossing_train = east_station;
            east_station = dequeue_train(east_station);

        }

        status = pthread_mutex_unlock(&schedule);
        check_unlocks(status);

        char *direction = "West";
        if(crossing_train -> direction == 'e'){
            direction = "East";

            if(last_dir == 'e'){
                /* if the direction of current train crossing is the same as
                   the direction of the last train that crossed */
                consecutive_trains++;
            }else{
                /* if the direction of the current train crossing is opposite
                   from the direction of the last train that crossed reset
                   consecutive_trains counter and update last_dir variable*/
                consecutive_trains = 1;
                last_dir = 'e';
            }
        }else{
            if(last_dir == 'w'){
                consecutive_trains++;
            }else{
                consecutive_trains = 1;
                last_dir = 'w';
            }
        }

        cross_track(crossing_train, direction);

        /* signaling that train threat should terminate */
        status = pthread_cond_signal(&crossing_train -> terminating_cond);
        check_signal(status);
        
        /* waiting for train thread to terminate */
        status = pthread_join(crossing_train -> train_thread, NULL);
        check_join(status);
        
        free(crossing_train);
    }
    return;
}

void init_time(){
    /* function that stores starting time of program into 
       start variable */
    if(clock_gettime(CLOCK_REALTIME, &start) == -1){
        perror("clock gettime error");
        exit(1);
    }
}

void init_conditions(){
    /* function that initializes conditions used in the program;
       calls a helper function to check that each condition was 
       initialized successfully */
    int init = pthread_cond_init(&start_loading, NULL);
    check_cond_init(init);
    init = pthread_cond_init(&job_available, NULL);
    check_cond_init(init);
    init = pthread_cond_init(&threads_created, NULL);
    check_cond_init(init);
}

void init_mutex(){
    /* initializing mutex used for thread scheduling
       in the program */
    int init = pthread_mutex_init(&schedule, NULL);
    if(init != 0){
        perror("pthread_mutex_init() error");
    }
    /* initializing mutex that controls printing so format is correct*/
    init = pthread_mutex_init(&printing, NULL);
    if(init != 0){
        perror("pthread_mutex_init() error");
    }
}

void destroy_conditions(){
    int status = pthread_cond_destroy(&start_loading);
    check_cond_destroy(status);
    status = pthread_cond_destroy(&job_available);
    check_cond_destroy(status);
    status = pthread_cond_destroy(&threads_created);
    check_cond_destroy(status);
}

void read_file_create_threads(FILE *filename){
    /* function that reads file; storing information of each line 
       in a Train node and creating a corrsponding thread.
       Returns the final Train structure created. */
    
    char direction;
    int loading_time;
    int crossing_time;
    int priority = 0;

    /* reading first line */
    int result = fscanf(filename, "%c %d %d\n", &direction, &loading_time, &crossing_time);

    Train *curr_train;

    while(result != EOF){
        /* setting priority to 1 if it is high */
        if(direction == 'E' || direction == 'W'){
            priority = 1;
        }
        /* storing direction as lowercase letters */
        if(direction == 'E'){
            direction = 'e';
        }else if(direction == 'W'){
            direction = 'w';
        }
        /* acquiring lock to updates lines_read variable */
        pthread_mutex_lock(&schedule);
        lines_read++;
        /* creating structure for storing current line's information */
        curr_train = new_train(lines_read-1, priority, direction, loading_time, crossing_time);
        
        pthread_mutex_unlock(&schedule);
        
        pthread_create(&curr_train -> train_thread, NULL, (void *)init_train, (void *)curr_train);

        /* reading next line */
        result = fscanf(filename, "%c %d %d\n", &direction, &loading_time, &crossing_time);

        /* resetting priority */
        priority = 0;
    }
    return;
}

int main(int argc, char *argv[]){
    init_time();
    init_mutex();
    init_conditions();
    
    FILE *filename = fopen(argv[1], "r");

    /* checking whether file could be opened */
    if(filename == NULL){
        perror("fopen() error");
        exit(1);
    }

    read_file_create_threads(filename);
    
    /* signaling to all trains to start loading */
    int status = pthread_mutex_lock(&schedule);
    check_locks(status);
    
    if(num_threads < lines_read){
        /* checking that all threads have been created, waits if not*/
        pthread_cond_wait(&threads_created, &schedule);
    }
    /* signaling for all train threads to start loading */
    int broadcast = pthread_cond_broadcast(&start_loading);
    if(broadcast != 0){
        perror("pthread_cond_broadcast() error");
    }
    
    status = pthread_mutex_unlock(&schedule);
    check_unlocks(status);

    schedule_and_cross();

    /* closing files, destroying mutex and exiting */
    fclose(filename);
    status = pthread_mutex_destroy(&schedule);
    check_mutex_destroy(status);
    status = pthread_mutex_destroy(&printing);
    check_mutex_destroy(status);
    destroy_conditions();
    return 0;
}