#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "libs/SharedMem.h"

extern void log_message();

void breakdown_manager() {
    log_message("Breakdown Manager: Process spawned\n");
    sleep(5);
    log_message("Breakdown Manager: Exiting\n");
    exit(0);
}