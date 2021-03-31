#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "libs/SharedMem.h"
#include "libs/TeamManager.h"

extern void log_message();
extern void wait_childs();
extern void init_proc();

void spawn_teams() {
    int* id;
    if (!(id = malloc(sizeof(int)))) {
        log_message("Call to malloc failed\n");
        log_message("Race Manager: Process exiting\n");
        wait_childs(configs->noTeams);
        exit(1);
    }
    for (int i = 0; i < configs->noTeams; i++) {
        *id = i+1;   
        init_proc(team_init, id);
    }
}

void init_boxes() {
    boxes_key = shmget(IPC_PRIVATE, sizeof(int)*configs->noTeams, IPC_EXCL|IPC_CREAT|0766);
    boxes = shmat(boxes_key, NULL, 0);
    for (int i = 0; i < configs->noTeams; i++) boxes[i] = 0;
}

void race_manager() {
    log_message("Race Manager: Process spawned\n");
    init_boxes();
    spawn_teams();
    wait_childs(configs->noTeams);
    log_message("Race Manager: Process exiting\n");
    exit(0);
}