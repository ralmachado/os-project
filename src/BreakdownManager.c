#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libs/SharedMem.h"

extern void log_message();

void breakdown_init() {
    log_message("Breakdown Manager process spawned\n");
    sleep(5);
    log_message("Breakdown Manager exiting\n");
    exit(0);
}