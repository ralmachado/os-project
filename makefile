CC = gcc
FLAGS = -Wall -Wextra -pthread -g
OBJS = src/RaceSimulator.o src/RaceManager.o src/TeamManager.o src/BreakdownManager.o src/Config.o
SRC = src/RaceSimulator.c src/RaceManager.c src/TeamManager.c src/BreakdownManager.c src/Config.c
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

src/RaceManager.o: src/RaceManager.c src/TeamManager.c

src/TeamManager.o: src/TeamManager.c

src/BreakdownManager.o: src/BreakdownManager.c
