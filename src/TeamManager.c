// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Team Manager process functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "libs/SharedMem.h"
#include "libs/MsgQueue.h"

pthread_t* cars;
sem_t* mutex;
int racers = 0, teamId, * box;
int pipe_fd;

extern void log_message();

// Car threads live here
void* vroom(void* r_id) {
    int id = *(int*)r_id;
    free(r_id);
    char buff[64];
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car thread #%d created", teamId, id);
    log_message(buff);
    Car *me = &(shm->teams[teamId-1]->cars[id-1]);
    me->fuel = configs.capacity;
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car #%d topped up", teamId, id);
    log_message(buff);
    sleep(2);
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car thread #%d exiting", teamId, id);
    
    log_message(buff);

    pthread_exit(0);
}

// Create a car thread if conditions match
void spawn_car() {
    if (racers < configs.maxCars) {
        int* id;
        if (!(id = malloc(sizeof(int)))) {
            log_message("malloc fail");
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
        snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Joined thread #%d", teamId, i + 1);
        log_message(buff);
    }

}

// Team Manager process lives here
void team_execute() {
    char buff[64];
    // snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Box state = %d", teamId, *box);
    // log_message(buff);
    for (int i = 0; i < 2; i++)
        spawn_car();

    join_threads();

    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Process exiting", teamId);
    log_message(buff);
    close(pipe_fd);
    exit(0);
}

// Setup Team Manager
void team_init(int id, int pipe) {
    pipe_fd = pipe;
    teamId = id;
    shm->teams[id-1]->id = id;
    write(pipe_fd, "FUCK", strlen("FUCK")+1);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Process spawned", teamId);
    log_message(buff);
    
    mutex = sem_open("MUTEX", 0);
    cars = malloc(sizeof(pthread_t) * configs.maxCars);
    // box = shm->boxes + (teamId - 1);

    team_execute();
}
