CC = gcc
LIBS = -lpthread -std=c11

main: obj.o
	$(CC) main.o -o dog $(LIBS)

obj.o: main.c types.h
	$(CC) -c main.c

clean:
	rm *.o dog