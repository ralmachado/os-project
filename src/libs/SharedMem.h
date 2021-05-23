#define BUFFSIZE 256
#define NAMESIZE 16

typedef enum boolean {false, true} bool;
typedef enum box {FREE, RESERVED, OCCUPPIED} Box;
typedef enum car_state {RACE, SAFETY, BOX, QUIT, FINISH} CarState;
typedef enum race_state {NS, ONGOING, END} RaceState;

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
    CarState state; // Car state
    bool lowFuel; // Has low fuel
    bool malfunction; // Is malfunctioning
    int number; // Car number
    int speed; // Car speed
    int reliability; // Car reliability
    int position; // Car position in track
    int laps; // Completed laps
    int stops; // Total pit stops
    int finish; // Leaderboard position once finished
    double consumption; // Car consumption rate
    double fuel; // Car fuel tank
    char *team;
} Car;

typedef struct team_struct {
    bool init; // Team is initialized?
    Box box; // Team's box state
    int racers; // Total racers
    int id; // TODO Replace all instances of id with name
    char name[NAMESIZE];
    Car *cars; // This is yet another shared memory
} Team;

typedef struct stats_struct {
    int malfunctions;
    int topup;
    int on_track;
    int racing;
} Stats;

typedef struct shared_struct {
    int pos;
    int init_cars;
    Stats stats;
    RaceState race_status;
    bool race_int; // SIGINT received
    bool race_usr1; // SIGUSR1 received
    bool show_stats; // Stop cars while showing stats
    Team *teams; // This is another shared memory
    pthread_cond_t race_cv, stats_cv;
    pthread_mutex_t race_mutex, stats_mutex;
} Sharedmem;

Config configs;
Sharedmem *shm;
Car** stats_arr;
int statsid;

void get_statistics();