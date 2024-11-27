CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
TARGET = elevator
SRC = main.c immeuble.c ascenseur.c
OBJ = $(SRC:.c=.o)

all: $(TARGET) controller

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

controller: controller.c
	$(CC) $(CFLAGS) -c controller.c -o controller.o
	$(CC) $(CFLAGS) -o $@ controller.o

clean:
	rm -f $(OBJ) $(TARGET) controller controller.o

.PHONY: all clean
