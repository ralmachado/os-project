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

#include "libs/header.h"
#include "libs/team.h"

#define PIPE_NAME "manager"

extern void log_message();
extern void wait_childs();
extern void init_proc();

int fd_npipe = -1;
int *upipes = NULL;
int *init_cars = NULL;
fd_set read_set;
sigset_t intmask;

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

int open_npipe() {
    if ((fd_npipe = open(PIPE_NAME, O_RDWR)) == -1) {
        log_message("[Race Manager] Failed to open named pipe");
        return 1;
    }
    log_message("[Race Manager] Open named pipe");
    return 0;
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
    new_car->team = team->name;

    stats_arr[*init_cars] = new_car; 

    snprintf(buff, sizeof(buff),
        "[Race Manager] New car => Team: %s, Car: %d, Speed: %d, Consumption: %.2f, Reliability: %d",
        team->name, new_car->number, new_car->speed, new_car->consumption, new_car->reliability);
    log_message(buff);
    (*init_cars)++;
}

void npipe_opts(char *opt) {
    char *saveptr;
    char buff[BUFFSIZE];
    if (strcmp(opt, "START RACE") == 0) {
        log_message("[Race Manager] Received START RACE command");
        if (shm->race_status != ONGOING && *init_cars > 0) {
            shm->race_status = ONGOING;
            shm->race_int = false;
            shm->race_usr1 = false;
            pthread_cond_broadcast(&(shm->race_cv));
            sigprocmask(SIG_BLOCK, &intmask, NULL);
            log_message("[Race Manager] Race started");
        } else if (shm->race_status == ONGOING) {
            log_message("[Race Manager] Race has already started");
        } else if (*init_cars == 0) {
            log_message("[Race Manager] Race can't start, no cars have been added");
        }
    } else if (strncmp(opt, "ADDCAR", 6) == 0) {
        log_message("[Race Manager] Received ADDCAR command");
        if (shm->race_status == ONGOING) {
            log_message("[Race Manager] Cannot add car: race already started");
            return;
        }
        char *token = strtok_r(opt+7, " ", &saveptr);
        char team_name[BUFFSIZE];
        int car, speed, reliability;
        double consumption;
        if (token) {
            strncpy(team_name, token, sizeof(team_name));
        }  else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        if ((token = strtok_r(NULL, " ", &saveptr))) {
            if ((car = atoi(token)) <= 0) {
                log_message("[Race Manager] Invalid car configuration received (car no.)");
                return;
            }
        } else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        if ((token = strtok_r(NULL, " ", &saveptr))) {
            if ((speed = atoi(token)) <= 0) {
                log_message("[Race Manager] Invalid car configuration received (speed)");
                return;
            }
        } else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        if ((token = strtok_r(NULL, " ", &saveptr))) {
            if ((consumption = atof(token)) <= 0) {
                log_message("[Race Manager] Invalid car configuration received (consumption)");
                return;
            }
        } else {
            log_message("[Race Manager] Invalid car configuration received (missing args)");
            return;
        }
        if ((token = strtok_r(NULL, " ", &saveptr))) {
            if ((reliability = atoi(token)) <= 0) {
                log_message("[Race Manager] Invalid car configuration received (reliability)");
                return;
            }
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
                nread = read(fd_npipe, buff, sizeof(buff));
                if (nread > 0) {
                    buff[nread-1] = 0;
                    npipe_opts(buff);
                }
                close(fd_npipe);
                open(PIPE_NAME, O_RDWR);
            }
            // Unnamed pipe activity handling
            for (i = 0; i < configs.noTeams; i++) {
                if (FD_ISSET(*(upipes+i), &read_set)) {
                    nread = read(*(upipes+i), buff, sizeof(buff));
                    if (nread > 0) {
                        buff[nread-1] = 0;
                        if (strncmp(buff, "RACE FINISH", 11) == 0) {                           
                            if (shm->race_usr1 || shm->race_int) {
                                get_statistics();
                                shm->race_usr1 = false;
                                shm->race_int = false;
                            }
                            pthread_mutex_lock(&(shm->race_mutex));
                            if (pthread_cond_broadcast(&(shm->race_cv))) 
                                log_message("[Race Manager] Failed to broadcast state change");
                            else log_message("[Race Manager] Broadcasted state change");
                            pthread_mutex_unlock(&(shm->race_mutex));
                            log_message("[Race Manager] Race finished");
                            sigprocmask(SIG_UNBLOCK, &intmask, NULL);
                        } else log_message(buff);
                    }
                }
            }
        } // if (select() > 0)
    } // while(1)
}

void stop_statistics(int signo) {
    if (signo == SIGUSR1) {
        if (shm->race_status == ONGOING) shm->race_usr1 = true;
        else log_message("[Race Manager] No race to interrupt");
    }
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

    sigemptyset(&intmask);
    sigaddset(&intmask, SIGINT);

    char buff[BUFFSIZE];
    snprintf(buff, sizeof(buff), "[Race Manager] Process spawned (PID %d)", mypid);
    log_message(buff);
    if (store_unnamed()) manager_term(1);
    if (open_npipe()) manager_term(1);
    shm->race_status = NS;
    
    init_cars = &(shm->init_cars);

    spawn_teams();
    pipe_listener();
    manager_term(0);
}
