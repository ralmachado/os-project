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
#include <unistd.h>
#include "libs/SharedMem.h"
#include "libs/TeamManager.h"

#define PIPE_NAME "manager"

extern void log_message();
extern void wait_childs();
extern void init_proc();

int fd_npipe;

// Create Team Manager processes
void spawn_teams() {
    int* id;
    if (!(id = malloc(sizeof(int)))) {
        log_message("[Race Manager] Call to malloc failed\n");
        log_message("[Race Manager] Process exiting\n");
        wait_childs(configs->noTeams);
        exit(1);
    }
    for (int i = 0; i < configs->noTeams; i++) {
        *id = i+1;
        init_proc(team_init, id);
    }
}

// Initialize noTeams boxes in shared memory
void init_boxes() {
    boxes_key = shmget(IPC_PRIVATE, sizeof(int)*configs->noTeams, IPC_EXCL|IPC_CREAT|0766);
    boxes = shmat(boxes_key, NULL, 0);
    for (int i = 0; i < configs->noTeams; i++) boxes[i] = 0;
    log_message("[Race Manager] Boxes shared memory created and attached\n");
}

void rm_boxes() {
    if (boxes) {
        shmctl(boxes_key, IPC_RMID, NULL);
        boxes = NULL;
        log_message("[Race Manager] Boxes shared memory destroyed\n");
    }
}

void test_npipe() {
  int nread;
  char buff[512], msg[512+26];
  do {
    if ((nread = read(fd_npipe, &buff, sizeof(buff))) > 0) {
      buff[nread-1] = 0;
      snprintf(msg, sizeof(msg), "[Race Manager] RECEIVED: %s\n", buff);
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
    log_message("[Race Manager] Failed to close named pipe\n");
  else log_message("[Race Manager] Closed named pipe\n");
}

// Race Manager process lives here
void race_manager() {
  signal(SIGINT, manage_int);
  log_message("[Race Manager] Process spawned\n");
  if (open_npipe()) {
    log_message("[Race Manager] Failed to open named pipe\n");
    log_message("[Race Manager] Process exiting\n");
    exit(0);
  }
  log_message("[Race Manager] Open named pipe\n");
  test_npipe();
  close_npipe();
  // init_boxes();
  // spawn_teams();
  // wait_childs(configs->noTeams);
  // rm_boxes();
  log_message("[Race Manager] Process exiting\n");
  exit(0);
}
