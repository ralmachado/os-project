// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Team Manager process functions

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/msg.h>
#include "libs/SharedMem.h"
#include "libs/MsgQueue.h"

pthread_t* cars, box_t;
pthread_mutex_t box_state = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t repair_cv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t repair_mutex= PTHREAD_MUTEX_INITIALIZER;
sem_t box_worker;
int racers = 0, * box;
int pipe_fd;
int in_box = -1;
Team *team;

extern void log_message();

void thread_exit() {
    pthread_exit(NULL);
}

// I have no idea if any of the code I am writing here will even work...
void race(Car *me, int id) {
    // TODO Receive breakdown messages
    // msg ohno;
    // msgrcv(mqid, &ohno, msglen, *FIXME*, IPC_NOWAIT); // TODO Find way to uniquely identify a car across all teams 

    if (me->fuel == 0 && !(me->state == QUIT)) {
        me->state = QUIT;
        // TODO Update team and make car stop race
        pause(); // Maybe just pause the thread?
    }

    if (me->state == RACE && (me->fuel) < 2*(me->consumption)*(configs.lapDistance)/(me->speed)) {
        me->state = SAFETY; 
    }
    
    if (!(me->lowFuel) && me->state == RACE && (me->fuel) < 4*(me->consumption)*(configs.lapDistance)/(me->speed) 
        && team->box == FREE) {
        me->lowFuel = true;
    }

    if (me->state == SAFETY && team->box == FREE) team->box == RESERVED;

    switch (me->state) {
        case RACE:
            me->fuel -= me->consumption;
            if (me->position + me->speed > configs.lapDistance) {
                if (++(me->laps) == configs.lapCount) {
                    me->state = FINISH;
                    pthread_exit(0);
                }
                if (me->lowFuel && team->box == FREE) { 
                    pthread_mutex_lock(&box_state);
                    if (team->box == FREE) { 
                        me->position = 0;
                        me->state = BOX;
                        team->box == OCCUPPIED;
                        in_box = id;
                        sem_post(&box_worker);
                        pthread_mutex_unlock(&box_state);
                        pthread_mutex_lock(&repair_mutex);
                        while (in_box != -1)
                            pthread_cond_wait(&repair_cv, &repair_mutex);
                        pthread_mutex_unlock(&repair_mutex);
                        me->state = RACE;
                    }
                    pthread_mutex_unlock(&box_state);
                }
            }
            me->position += me->speed;
            me->position %= configs.lapDistance;
            break;
        case SAFETY:
            me->fuel -= 0.4*(me->consumption);
            if (me->position + 0.3 * (me->speed) > configs.lapDistance && ++(me->laps) <= configs.lapCount) {
                pthread_mutex_lock(&box_state);
                if (team->box == RESERVED) {
                    me->position = 0;
                    me->state = BOX;
                    team->box == OCCUPPIED;
                    in_box = id;
                    sem_post(&box_worker);
                    pthread_mutex_unlock(&box_state);
                    pthread_mutex_lock(&repair_mutex);
                    while (in_box != -1)
                        pthread_cond_wait(&repair_cv, &repair_mutex);
                    pthread_mutex_unlock(&repair_mutex);
                    me->state = RACE;
                }
                pthread_mutex_unlock(&box_state);
            }
            me->position += 0.3 * me->speed;
            me->position %= configs.lapDistance;
            break;
    }

    sleep(configs.timeUnit);
}

// Car threads live here
void* vroom(void* r_id) {
    int id = *(int*)r_id;
    free(r_id);
    char buff[64];
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car thread #%d created", team->id, id);
    log_message(buff);
    Car *me = &(team->cars[id-1]);
    // FIX delegate initialization to named pipe late
    // me->fuel = configs.capacity;
    // me->position = 0;
    // snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car #%d topped up", team->id, id);
    // log_message(buff);
    sleep(2); // TODO Don't forget to get rid of this sleep
    // Race function (race())
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Car thread #%d exiting", team->id, id);
    
    log_message(buff);

    pthread_exit(0);
}

// TODO don't forget to spawn thread
void* car_box() {
    char buff[BUFFSIZE];
    snprintf(buff, sizeof(buff), "[Team Manager #%d] Box is open", team->id);
    log_message(buff);
    struct sigaction sig;
    sig.sa_handler = thread_exit;
    sigemptyset(&sig.sa_mask);
    sig.sa_flags = 0;
    sigaction(SIGINT, &sig, NULL);
    
    srand(pthread_self());
    while (1) {
        sem_wait(&box_worker);
        Car *boxed = &(team->cars[in_box-1]);
        boxed->stops++;

        if (boxed->lowFuel) {
            boxed->fuel = configs.capacity; 
            boxed->lowFuel = false;
            shm->topup++;
        }

        if (boxed->malfunction) {
            boxed->malfunction = false;
            shm->malfunctions++;
        }

        sleep(rand() % (configs.tBoxMax + 1) + configs.tBoxMin);
        in_box = -1;
        pthread_cond_signal(&repair_cv);
    }
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
        snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Joined thread #%d", team->id, i + 1);
        log_message(buff);
    }

}

void team_destroy() {
    char buff[BUFFSIZE];

    if (close(pipe_fd)) {
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Failed to close unnamed pipe", team->id);
        log_message(buff);
    } else {
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Closed unnamed pipe", team->id);
        log_message(buff);
    }

    if (pthread_mutex_destroy(&box_state)) {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Failed to destroy mutex 'box_state'", team->id);
        log_message(buff);
    } else {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Destroyed mutex 'box_state'", team->id);
        log_message(buff);
    }
    
    if (pthread_mutex_destroy(&repair_mutex)) {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Failed to destroy mutex 'repair_mutex'", team->id);
        log_message(buff);
    } else {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Destroyed mutex 'repair_mutex'", team->id);
        log_message(buff);
    }
    
    if (pthread_cond_destroy(&repair_cv)) {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Failed to destroy condition variable 'repair_cv'", team->id);
        log_message(buff);
    } else {
        snprintf(buff, sizeof(buff), 
            "[Team Manager #%d] Destroyed condition variable 'repair_cv'", team->id);
        log_message(buff);
    }

    if (sem_destroy(&box_worker)) {
        snprintf(buff, sizeof(buff), "[Team Manager #%d] Failed to destroy 'box_worker' semaphore", team->id);
        log_message(buff);
    }
}

// FIX create signal handler

// Team Manager process lives here
void team_execute() {
    char buff[BUFFSIZE];
    for (int i = 0; i < 2; i++)
        spawn_car();

    pthread_create(&box_t, NULL, car_box, NULL);
    sleep(2);
    pthread_kill(box_t, SIGINT);
    pthread_join(box_t, NULL);
    snprintf(buff, sizeof(buff), "[Team Manager #%d] Box closed", team->id);
    log_message(buff);
    join_threads();


    team_destroy();
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Process exiting", team->id);
    log_message(buff);
    exit(0);
}

// Setup Team Manager
void team_init(int id, int pipe) {
    pipe_fd = pipe;
    team = shm->teams[id-1];
    team->id = id;
    team->box = FREE;
    write(pipe_fd, "FUCK", strlen("FUCK")+1);
    char buff[128];
    snprintf(buff, sizeof(buff) - 1, "[Team Manager #%d] Process spawned", team->id);
    log_message(buff);
    
    cars = malloc(sizeof(pthread_t) * configs.maxCars);

    sem_init(&box_worker, 0, 0);
    
    team_execute();
}
