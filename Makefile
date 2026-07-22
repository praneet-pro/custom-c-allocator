CC = gcc
CFLAGS = -Wall -Wextra -g

all: benchmark

benchmark: benchmark.c free_list.c free_list.h
	$(CC) $(CFLAGS) benchmark.c free_list.c -o benchmark

clean:
	rm -f benchmark