FLAGS = -Wall -g
LIBS = -lm

# Headers
DEPENDENCIES = model.h calcs.h brightness.h rotation.h

# Final executable
all: main

main: main.o calcs.o brightness.o rotation.o
	gcc ${FLAGS} -o $@ $^ ${LIBS}

main.o: main.c ${DEPENDENCIES}
	gcc ${FLAGS} -c $<

calcs.o: calcs.c calcs.h model.h brightness.h rotation.h
	gcc ${FLAGS} -c $<

brightness.o: brightness.c brightness.h model.h
	gcc ${FLAGS} -c $<

rotation.o: rotation.c rotation.h model.h
	gcc ${FLAGS} -c $<

clean:
	rm -f *.o main *.gch
