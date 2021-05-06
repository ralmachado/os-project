#define BUFFSIZE 256
#define true 1
#define false 0

typedef struct config_struct {
    int timeUnit;
    int lapDistance;
    int lapCount;
    int noTeams;
    int maxCars;
    int tBreakdown;
    int tBoxMin;
    int tBoxMax;
    int capacity;
} Config;

typedef struct car_struct {
    short int state;
    short int lowFuel;
    short int malfunction;
    int speed;
    int consumption;
    int reliability;
    int position;
    int laps;
    int fuel;
    int stops;
} Car;

typedef struct team_struct {
    int id;
    int racers;
    short int box;
    Car *cars;
} Team;

typedef struct shared_struct {
    int malfunctions;
    int topup;
    Team **teams;
} Sharedmem;

enum Box{FREE, RESERVED, OCCUPPIED};
enum State{RACE, SAFETY, BOX, QUIT, FINISH};

Config configs;
Sharedmem *shm;
int shmid;
