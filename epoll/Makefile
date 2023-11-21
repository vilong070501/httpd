CC = gcc
CFLAGS = -std=c99 -Werror -Wall -Wextra -Wvla -pedantic -g

OBJ = epoll.o
BINARY = epoll

all: $(BINARY)

epoll: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	$(RM) $(OBJ)
	$(RM) $(BINARY)
