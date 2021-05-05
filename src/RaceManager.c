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
#include "libs/UnnamedPipes.h"

#define PIPE_NAME "manager"

extern void log_message();
extern void wait_childs();
extern void init_proc();

int fd_npipe = -1;
int(*pipes)[2] = NULL;

void manager_term(int code) {
    while (wait(NULL) != -1);
    if (fd_npipe != -1) close(fd_npipe);
    if (pipes) shmctl(pipes_id, IPC_RMID, NULL);
    if (boxes) shmctl(boxes_key, IPC_RMID, NULL);
    if (code == -1) exit(code);
    exit(0);
}

int store_unnamed() {
    pipes_id = shmget(IPC_PRIVATE, sizeof(configs->noTeams) * sizeof(int[2]), IPC_CREAT | 0600);
    if (pipes_id == -1) {
        log_message("[Race Manager] Failed to get unnamed pipes shm identifier");
        return 0;
    } else if (!(pipes = shmat(pipes_id, NULL, 0))) {
        log_message("[Race Manager] Failed to open unnamed pipes storage");
        return 0;
    }
    log_message("[Race Manager] Opened unnamed pipes storage");
    return 1;
}

// Create Team Manager processes
void spawn_teams() {
    for (int i = 0; i < configs->noTeams; i++) {
        if (pipe(*(pipes + i))) {
            log_message("[Race Manager] Failed to open unnamed pipe");
            manager_term(1);
        }
        if (fork() == 0) {
            team_init(i+1);
        }
    }
}

// Initialize noTeams boxes in shared memory
void init_boxes() {
    boxes_key = shmget(IPC_PRIVATE, sizeof(int) * configs->noTeams, IPC_EXCL | IPC_CREAT | 0766);
    boxes = shmat(boxes_key, NULL, 0);
    for (int i = 0; i < configs->noTeams; i++) boxes[i] = 0;
    log_message("[Race Manager] Boxes shared memory created and attached");
}

void rm_boxes() {
    if (boxes) {
        shmctl(boxes_key, IPC_RMID, NULL);
        boxes = NULL;
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

int close_npipe() {
    return close(fd_npipe);
}

void manage_int(int signum) {
    if (close_npipe())
        log_message("[Race Manager] Failed to close named pipe");
    else log_message("[Race Manager] Closed named pipe");
}

// Race Manager process lives here
void race_manager() {
    struct sigaction sigint;
    sigint.sa_handler = manage_int;
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
    if (!store_unnamed()) manager_term(1);
    init_boxes();
    spawn_teams();
    //   wait_childs();
    //   rm_boxes();
    test_npipe();
    //   close_npipe();
    manager_term(0);
    log_message("[Race Manager] Process exiting");
    exit(0);
}
