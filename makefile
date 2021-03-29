CC = gcc
FLAGS = -Wall -Wextra -pthread
OBJS = main.o sharedmem.o team.o breakdown.o
SRC = main.c sharedmem.c team.c breakdown.c
OUT = main

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

main.o: ${SRC}

team.o: team.c sharedmem.c

breakdown.o: breakdown.c sharedmem.c

sharedmem.o: sharedmem.c
