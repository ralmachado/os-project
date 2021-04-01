#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libs/SharedMem.h"

extern void log_message();

char buff[64];

void breakdown_manager() {
    log_message("Breakdown Manager: Process spawned\n");
    snprintf(buff, sizeof(buff)-1, "Breakdown Manager: tBreakdown -> %d\n", configs->tBreakdown);
    log_message(buff);
    snprintf(buff, sizeof(buff)-1, "Breakdown Manager: tBoxMin -> %d\n", configs->tBoxMin);
    log_message(buff);
    snprintf(buff, sizeof(buff)-1, "Breakdown Manager: tBoxMax -> %d\n", configs->tBoxMax);
    log_message(buff);
    sleep(5);
    log_message("Breakdown Manager: Process exiting\n");
    exit(0);
}