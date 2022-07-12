CC=gcc
CFLAGS=-Wall -g -Wextra
EXE=server
OBJ=serverops.o connops.o queue.o
LINK=-lpthread

$(EXE): server.c $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $< $(OBJ) $(LINK)

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o $(EXE)