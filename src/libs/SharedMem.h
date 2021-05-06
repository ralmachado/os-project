#define BUFFSIZE 256

typedef struct {
    int timeUnit, lapDistance, lapCount, noTeams, maxCars, tBreakdown, tBoxMin, tBoxMax, capacity;
} Config;

typedef struct {
    short int state;
    int speed;
    int consumption;
    int reliability;
    int fuel;
    int position;
    int laps;
    int stops;
    int malfunctions;
    int topup;
} Car;

typedef struct {
    int id;
    int racers;
    int box;
    Car *cars;
} Team;

typedef struct {
    Team **teams;
} Sharedmem;

enum Box{FREE, RESERVED, OCCUPPIED};
enum State{RACE, SAFETY, BOX, QUIT, FINISH};

Config configs;
Sharedmem *shm;
int shmid;
