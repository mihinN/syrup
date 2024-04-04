.DEFAULT_GOAL := all

CC=gcc

clean: 
	rm -rf *.o
	rm -rf *.out

compile: 
	$(CC) -c syp.c mpc.c 

link: 
	$(CC) -o syp syp.o mpc.o -lm -ledit

all: clean compile link

