CFLAGS  = -Wall -Werror -pedantic -fsanitize=address,undefined
SRC     = main.c ../hashmap.c ../iobuf.c ../http.c ../string.c
CC      = gcc

all: $(SRC)
	$(CC) $(CFLAGS) $^

fast:
	$(CC) -Wall -Werror -pedantic $(SRC)
