// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Race Manager process functions

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "libs/SharedMem.h"
#include "libs/TeamManager.h"

#define PIPE_NAME "manager"

extern void log_message();
extern void wait_childs();
extern void init_proc();

int fd_npipe = -1;
int *upipes = NULL;
int init_cars = 0;
fd_set read_set;
pthread_mutex_t *race_mutex;
pthread_cond_t *race_cv;

void manager_term(int code) {
    while (wait(NULL) != -1);

    if (fd_npipe != -1) {
        if (close(fd_npipe)) log_message("[Race Manager] Failed to close named pipe");
        else log_message("[Race Manager] Closed named pipe");
    }
    if (upipes) {
        for (int i = 0; i < configs.noTeams; i++) close(*(upipes+i));
        free(upipes);
        log_message("[Race Manager] Closed unnamed pipes");
    }

    if (pthread_mutex_destroy(&(shm->race_mutex)))
        log_message("[Race Manager] Failed to destroy 'race_mutex'");
    else log_message("[Race Manager] Destroyed 'race_mutex'");

    if (pthread_cond_destroy(race_cv))
        log_message("[Race Manager] Failed to destroy 'race_cv'");
    else log_message("[Race Manager] Destroyed 'race_cv'");

    log_message("[Race Manager] Process exiting");
    exit(code);
}

void manager_int(int signum) {
    if (signum == SIGINT) {
        manager_term(0);
    }
}

int store_unnamed() {
    upipes = malloc(sizeof(configs.noTeams)*sizeof(int));
    if (!upipes) {
        log_message("[Race Manager] Failed to allocate pipe descriptor array");
        return -1;
    }
    log_message("[Race Manager] Opened unnamed pipes storage");
    return 0;
}

// Create Team Manager processes
void spawn_teams() {
    int fd[2] = {-1, -1};
    for (int i = 0; i < configs.noTeams; i++) {
        if (pipe(fd)) {
            log_message("[Race Manager] Failed to open unnamed pipe");
            manager_term(1);
        }
        if (fork() == 0) {
            // Tenho os fd dos pipes em shared memory, mas uma alternativa seria guardar num malloc
            // e passar por argumento para o novo processo?
            close(fd[0]);
            team_init(i+1, fd[1]);
        }
        close(fd[1]);
        *(upipes+i) = fd[0];
    }
}

void test_npipe() {
    int nread;
    char buff[512], msg[512 + 26];
    do {
        if ((nread = read(fd_npipe, &buff, sizeof(buff))) > 0) {
            buff[nread - 1] = 0;
            snprintf(msg, sizeof(msg), "[Race Manager] RECEIVED: %s", buff);
            log_message(msg);
        }
    } while (strcmp(buff, "CLOSE") != 0);
}

int open_npipe() {
    if ((fd_npipe = open(PIPE_NAME, O_RDWR)) == -1) {
        log_message("[Race Manager] Failed to open named pipe");
        return 1;
    }
    log_message("[Race Manager] Open named pipe");
    return 0;
}

void test_pipes() {
    fd_set teams;
    FD_ZERO(&teams);
    for (int i = 0; i < configs.noTeams; i++)
        FD_SET(*(upipes+i), &teams);

    char buff[64], log[512];
    int n = read(*(upipes), buff, sizeof(buff));
    if (n > 0) {
        buff[n] = 0;
        snprintf(log, sizeof(log), "[Race Manager] Car 1 says: %s", buff);
        log_message(log);
    }
}

Team* find_team(char *team_name) {
    for (int i = 0; i < configs.noTeams; i++) {
        if (shm->teams[i].init == false) {
            shm->teams[i].init = true;
            strncpy(shm->teams[i].name, team_name, sizeof(shm->teams[i].name));
            return &(shm->teams[i]);
        } else if (strcmp(shm->teams[i].name, team_name) == 0) return &(shm->teams[i]);
    }
    return NULL;
}

int find_car(int car_no) {
    int i, j;
    Team* team;
    Car* cars;
    for (i = 0; i < configs.noTeams; i++) {
        team = &(shm->teams[i]);
        cars = team->cars;
        for (j = 0; j < team->racers; j++) {
            if (cars[j].number == car_no) return 1;
        }
    }
    return 0;
}

void add_car(char *team_name, int car, int speed, double consumption, int reliability) {
    char buff[BUFFSIZE];
    Team *team;
    if (!(team = find_team(team_name))) {
        snprintf(buff, sizeof(buff), "[Race Manager] Couldn't find or allocate team %s", team_name);
        log_message(buff);
        return;
    }

    if (find_car(car)) {
        snprintf(buff, sizeof(buff), "[Race Manager] Car %d already exists", car);
        log_message(buff);
        return;
    }

    if (team->racers == configs.maxCars) {
        snprintf(buff, sizeof(buff), "[Race Manager] Team %s is already full", team_name);
        log_message(buff);
        return;
    }

    Car *new_car = &(team->cars[(team->racers)++]);
    new_car->number = car;
    new_car->speed = speed;
    new_car->reliability = reliability;
    new_car->consumption = consumption;
    new_car->fuel = configs.capacity;
    new_car->lowFuel = false;
    new_car->malfunction = false;
    new_car->position = 0;
    new_car->stops = 0;

    snprintf(buff, sizeof(buff),
        "[Race Manager] New car => Team: %s, Car: %d, Speed: %d, Consumption: %.2f, Reliability: %d",
        team->name, new_car->number, new_car->speed, new_car->consumption, new_car->reliability);
    log_message(buff);
    init_cars++;
}

void npipe_opts(char *opt) {
    char *saveptr;
    char buff[BUFFSIZE];
    if (strcmp(opt, "START RACE") == 0) {
        log_message("[Race Manager] Received START RACE command");
        if (shm->race_status == false && init_cars > 0) {
            shm->race_status = true;
            pthread_cond_broadcast(&(shm->race_cv));
            log_message("[Race Manager] Race started");
        } else if (shm->race_status == true) {
            log_message("[Race Manager] Race has already started");
        } else if (init_cars == 0) {
            log_message("[Race Manager] Race can't start, no cars have been added");
        }
    } else if (strncmp(opt, "ADDCAR", 6) == 0) {
        log_message("[Race Manager] Received ADDCAR command");
        if (shm->race_status == true) {
            log_message("[Race Manager] Cannot add car: race already started");
            return;
        }
        // printf("opt = %s\n", opt);
        char *token = strtok_r(opt+7, " ", &saveptr);
        char team_name[BUFFSIZE];
        int car, speed, reliability;
        double consumption;
        if (token) {
            strncpy(team_name, token, sizeof(team_name));
            // printf("token = %s\n", token);
        }  else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        if ((token = strtok_r(NULL, " ", &saveptr))) {
            if ((car = atoi(token)) <= 0) {
                log_message("[Race Manager] Invalid car configuration received (car no.)");
                return;
            }
            // printf("token = %s | car = %d\n", token, car);
        } else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        if ((token = strtok_r(NULL, " ", &saveptr))) {
            if ((speed = atoi(token)) <= 0) {
                log_message("[Race Manager] Invalid car configuration received (speed)");
                return;
            }
            // printf("token = %s | speed = %d\n", token, speed);
        } else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        if ((token = strtok_r(NULL, " ", &saveptr))) {
            if ((consumption = atof(token)) <= 0) {
                log_message("[Race Manager] Invalid car configuration received (consumption)");
                return;
            }
            // printf("token = %s | consumption = %.2f\n", token, consumption);
        } else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        if ((token = strtok_r(NULL, " ", &saveptr))) {
            if ((reliability = atoi(token)) <= 0) {
                log_message("[Race Manager] Invalid car configuration received (reliability)");
                return;
            }
            // printf("token = %s | reliability = %d\n", token, reliability);
        } else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        add_car(team_name, car, speed, consumption, reliability);
    } else {
        snprintf(buff, sizeof(buff), "[Race Manager] Invalid command: %s", opt);
        log_message(buff);
    }
}

void pipe_listener() {
    log_message("[Race Manager] Listening on all pipes");
    int i, nread;
    char buff[BUFFSIZE];
    while (1) {
        FD_ZERO(&read_set);
        FD_SET(fd_npipe, &read_set);
        for (i = 0; i < configs.noTeams; i++)
            FD_SET(upipes[i], &read_set);

        // Named pipe activity handling
        if (select(upipes[configs.noTeams-1]+1, &read_set, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(fd_npipe, &read_set)) {
                log_message("[Race Manager] Activity on named pipe"); // FIXME Delete so prof doesn't see this
                do {
                    nread = read(fd_npipe, buff, sizeof(buff));
                    if (nread > 0) {
                        buff[nread-1] = 0;
                        npipe_opts(buff);
                    }
                } while (nread > 0);
                close(fd_npipe);
                open(PIPE_NAME, O_RDWR);
            }
            // Unnamed pipe activity handling
            for (i = 0; i < configs.noTeams; i++) {
                if (FD_ISSET(*(upipes+i), &read_set)) {
                    // TODO Get updates from Teams
                    snprintf(buff, sizeof(buff), "[Race Manager] Activity on unnamed pipe %d", i);
                    log_message(buff);
                    nread = read(*(upipes+i), buff, sizeof(buff));
                    if (nread > 0) {
                        buff[nread-1] = 0;
                        log_message(buff);
                    }
                }
            }
        } // if (select() > 0)
    } // while(1)
}

void stop_statistics(int signo) {
    if (signo == SIGUSR1)
        log_message("[Race Manager] Caught SIGUSR1, not yet implemented");
}

// Race Manager process lives here
void race_manager() {
    pid_t mypid = getpid();
    struct sigaction sigint, sigusr;
    sigint.sa_handler = manager_int;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = 0;
    sigaction(SIGINT, &sigint, NULL);

    sigusr.sa_handler = stop_statistics;
    sigusr.sa_flags = SA_NODEFER;
    sigemptyset(&sigusr.sa_mask);
    sigaction(SIGUSR1, &sigusr, NULL);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    char buff[BUFFSIZE];
    snprintf(buff, sizeof(buff), "[Race Manager] Process spawned (PID %d)", mypid);
    log_message(buff);
    if (store_unnamed()) manager_term(1);
    if (open_npipe()) manager_term(1);
    shm->race_status = false;
    race_mutex = &(shm->race_mutex);
    race_cv = &(shm->race_cv);
    pthread_condattr_t shared_cv;
    pthread_mutexattr_t shared_mutex;
    pthread_condattr_init(&shared_cv);
    pthread_mutexattr_init(&shared_mutex);
    pthread_condattr_setpshared(&shared_cv, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setpshared(&shared_mutex, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&shared_mutex, PTHREAD_MUTEX_ROBUST);
    if (pthread_cond_init(race_cv, &shared_cv))
        log_message("[Race Manager] Failed to initialize 'race_cv'");
    else log_message("[Race Manager] Initialized 'race_cv'");

    if (pthread_mutex_init(race_mutex, &shared_mutex))
        log_message("[Race Manager] Failed to initialize 'race_mutex'");
    else log_message("[Race Manager] Initialized 'race_mutex'");

    spawn_teams();
    // test_pipes();
    pipe_listener();
    manager_term(0);
}
