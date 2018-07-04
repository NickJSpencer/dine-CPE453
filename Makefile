CC = gcc
CFLAGS = -Wall -g
LD = gcc
LDFLAGS = -Wall -g -L.
LIBS = -lpthread -lrt

dine: dine.o
	$(CC) -o dine dine.o $(LIBS)
   
dine.o: dine.c 
	$(CC) $(CFLAGS) -c dine.c