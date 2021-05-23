CC = gcc
FLAGS = -Wall -Wextra -pthread -g
OBJS = src/simulator.o src/race.o src/team.o src/breakdown.o src/config.o
SRC = src/simulator.c src/race.c src/team.c src/breakdown.c src/config.c
OUT = RaceSimulator

${OUT}: ${OBJS}
	${CC} ${FLAGS} ${OBJS} -o $@

clean:
	rm -f ${OBJS}

debug:
	${CC} -g -c ${SRC}
	${CC} ${FLAGS} -g ${OBJS} -o ${OUT}

%.o: %.c
	${CC} ${FLAGS} -c $< -o $@

####################################

src/RaceSimulator.o: ${SRC} 

src/RaceManager.o: src/race.c src/team.c

src/TeamManager.o: src/team.c

src/BreakdownManager.o: src/breakdown.c
