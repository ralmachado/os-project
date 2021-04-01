#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "libs/SharedMem.h"

pthread_t* cars;
sem_t* mutex;
int racers = 0, teamId, * box;

extern void log_message();

// Car threads live here
void* vroom(void* r_id) {
    int id = *(int*)r_id;
    free(r_id);
    char buff[64];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Car thread #%d created\n", teamId, id);
    sem_wait(mutex);
    log_message(buff);
    snprintf(buff, sizeof(buff)-1, "Car #%d/%d goes vroom!\n", id, teamId);
    log_message(buff);
    sem_post(mutex);
    sleep(2);
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Car thread #%d exiting\n", teamId, id);
    
    sem_wait(mutex);
    log_message(buff);
    sem_post(mutex);

    pthread_exit(0);
}

// Create a car thread if conditions match
void spawn_car() {
    if (racers < configs->maxCars) {
        int* id;
        if (!(id = malloc(sizeof(int)))) {
            log_message("malloc fail\n");
            return;
        }
        *id = racers + 1;
        pthread_create(&cars[racers], NULL, vroom, id);
        racers++;
    }
}

// Await for all car threads to exit
void join_threads() {
    char buff[64];
    for (int i = 0; i < racers; i++) {
        pthread_join(cars[i], NULL);
        snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Joined thread #%d\n", teamId, i + 1);
        sem_wait(mutex);
        log_message(buff);
        sem_post(mutex);
    }

}

// Team Manager process lives here
void team_execute() {
    char buff[64];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Box state = %d\n", teamId, *box);
    sem_wait(mutex);
    log_message(buff);
    sem_post(mutex);
    for (int i = 0; i < 2; i++)
        spawn_car();

    join_threads();

    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Process exiting\n", teamId);
    log_message(buff);
    exit(0);
}

// Setup Team Manager
void team_init(void* id) {
    teamId = *(int*)id;
    free(id);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "Team Manager #%d: Process spawned\n", teamId);
    sem_wait(mutex);
    log_message(buff);
    sem_post(mutex);
    
    mutex = sem_open("MUTEX", 0);
    cars = malloc(sizeof(pthread_t) * configs->maxCars);
    box = boxes + (teamId - 1);

    team_execute();
}
