// David Valente Pereira Barros Leitão - 2019223148
// Rodrigo Alexandre da Mota Machado - 2019218299

// Race Manager process functions

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
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
int *pipes = NULL;

void manager_term(int code) {
    while (wait(NULL) != -1);
    if (fd_npipe != -1) close(fd_npipe);
    if (pipes) free(pipes);
    if (shm->boxes) free(shm->boxes);
    if (code == -1) exit(code);
    else if (code == SIGINT) exit(1);
    exit(0);
}

int store_unnamed() {
    pipes = malloc(sizeof(configs.noTeams)*sizeof(int));
    if (!pipes) {
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
        *(pipes+i) = fd[0];
    }
}

// Initialize noTeams boxes in shared memory
int init_boxes() {
    shm->boxes = calloc(configs.noTeams, sizeof(int));
    if (shm->boxes) {
        for (int i = 0; i < configs.noTeams; i++) shm->boxes[i] = 0;
        log_message("[Race Manager] Boxes created");
        return 0;
    } 
    log_message("[Race Manager] Failed to init boxes");
    return -1;
}

void rm_boxes() {
    if (shm->boxes) {
        free(shm->boxes);
        shm->boxes = NULL;
        log_message("[Race Manager] Boxes shared memory destroyed");
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
    if ((fd_npipe = open(PIPE_NAME, O_RDWR)) == -1)
        return 1;
    return 0;
}

void test_pipes() {
    fd_set teams;
    FD_ZERO(&teams);
    for (int i = 0; i < configs.noTeams; i++)
        FD_SET(*(pipes+i), &teams);

    char buff[64], log[512];
    int n = read(*(pipes), buff, sizeof(buff));
    if (n > 0) {
        buff[n] = 0;
        snprintf(log, sizeof(log), "[Race Manager] Car 1 says: %s", buff);
        log_message(log);
    }
}

// Race Manager process lives here
void race_manager() {
    struct sigaction sigint;
    sigint.sa_handler = manager_term;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = 0;
    sigaction(SIGINT, &sigint, NULL);
    log_message("[Race Manager] Process spawned");
    if (open_npipe()) {
        log_message("[Race Manager] Failed to open named pipe");
        log_message("[Race Manager] Process exiting");
        exit(0);
    }

    log_message("[Race Manager] Open named pipe");
    if (store_unnamed()) manager_term(1);
    if (init_boxes()) manager_term(1);
    spawn_teams();
    test_pipes();
    manager_term(0);
    log_message("[Race Manager] Process exiting");
    exit(0);
}
