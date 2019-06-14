CC=gcc-9
CFLAGS=-std=c11 -Wall -g

9cc: 9cc.c

test: 9cc
				./test.sh

clean:
				rm -rf 9cc *.o *~ tmp* *.dSYM
