typedef struct mem_struct {
    int timeUnit, lapDistance, lapCount, noTeams, tBreakdown, tBoxMin, tBoxMax, capacity;
} sharedmem;

sharedmem* configs;
int shm_key;
