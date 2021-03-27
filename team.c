#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void log_message();

void team_init() {
    log_message("Team Manager process spawned\n");
    team_execute();
}

void team_execute() {
    sleep(10);
    exit(0);
}