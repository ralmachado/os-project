CC = gcc
FLAGS = -Wall -Wextra -pthread
OBJS = src/RaceSimulator.o src/TeamManager.o src/BreakdownManager.o src/SharedMem.o
SRC = src/RaceSimulator.c src/TeamManager.c src/BreakdownManager.c src/SharedMem.c 
OUT = RaceSimulator

${OUT}: ${OBJS}
	${CC} ${FLAGS} ${OBJS} -o $@

clean:
	rm ${OBJS}

debug:
	${CC} -g -c ${SRC}
	${CC} ${FLAGS} -g ${OBJS} -o ${OUT}

.c .o:
	${CC} -c $< -o $@

####################################

src/RaceSimulator.o: ${SRC}

src/TeamManager.o: src/TeamManager.c src/SharedMem.c

src/BreakdownManager.o: src/BreakdownManager.c src/SharedMem.c

src/SharedMem.o: src/SharedMem.c
