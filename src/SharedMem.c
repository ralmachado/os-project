#include <stdlib.h>
#include <semaphore.h>

typedef struct mem_struct sharedmem;

sharedmem* configs = NULL;
int* boxes = NULL;
sem_t *mutex = NULL;