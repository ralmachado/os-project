// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Race Manager process functions

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/select.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "libs/SharedMem.h"
#include "libs/TeamManager.h"

#define PIPE_NAME "manager"

extern void log_message();
extern void wait_childs();
extern void init_proc();

int fd_npipe = -1;
int *upipes = NULL;
fd_set read_set;

void manager_term(int code) {
    while (wait(NULL) != -1);
    if (fd_npipe != -1) close(fd_npipe);
    if (upipes) free(upipes);
    log_message("[Race Manager] Process exiting");
    exit(code);
}

void manager_int(int signum) {
    if (signum == SIGINT) manager_term(0);
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
    for (int i = 0; i < configs.noTeams; i++) {
        int fd[2];
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

void npipe_opts(char *opt, int size) {
    if (strcmp(opt, "START RACE!") == 0) {
        // TODO start race if not started yet, else complain!
    } else {
        char *token = strtok_r(opt, " ", &opt);
        if (strcmp(token, "ADDCAR ") == 0) {
            // TODO if race already started reject
            // TODO else keep tokenizing and add new car
            token = strtok_r(opt, "TEAM: ", &opt);
            token = strtok_r(opt, "CAR: ", &opt);
            token = strtok_r(opt, "SPEED: ", &opt);
            token = strtok_r(opt, "CONSUMPTION: ", &opt);
            token = strtok_r(opt, "RELIABILITY: ", &opt);
        }
    }
}

void pipe_listener() {
    int i, nread;
    char buff[BUFFSIZE];
    while (1) {
        FD_ZERO(&read_set);
        for (i = 0; i < configs.noTeams; i++)
            FD_SET(*(upipes+i), &read_set);
        
        if (select(*(upipes+configs.noTeams), &read_set, NULL, NULL, NULL)) {
            if (FD_ISSET(fd_npipe, &read_set)) { 
                // TODO Implement Named Pipe commands
                nread = read(fd_npipe, buff, sizeof(buff));
                if (nread > 0) {
                    buff[nread-1] = 0;
                    npipe_opts(buff, sizeof(buff)); // todo Implementing this
                }
            }
            
            for (i = 0; i < configs.noTeams; i++) {
                if (FD_ISSET(*(upipes+i), &read_set)) {
                    // TODO Get updates from Teams 
                }
            }
        }
    }
}

// Race Manager process lives here
void race_manager() {
    struct sigaction sigint;
    sigint.sa_handler = manager_int;
    sigemptyset(&sigint.sa_mask);
    sigaddset(&sigint.sa_mask, SIGTSTP);
    sigint.sa_flags = 0;
    sigaction(SIGINT, &sigint, NULL);

    log_message("[Race Manager] Process spawned");
    if (open_npipe()) manager_term(1);
    if (store_unnamed()) manager_term(1);
    spawn_teams();
    test_pipes();
    // pipe_listener();
    manager_term(0);
}
