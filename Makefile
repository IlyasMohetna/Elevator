CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
TARGET = elevator
SRC = main.c immeuble.c ascenseur.c
OBJ = $(SRC:.c=.o)

all: $(TARGET) controller virtualization

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

controller: controller.o immeuble.o
	$(CC) $(CFLAGS) -o $@ controller.o immeuble.o

virtualization: virtualization.o ascenseur.o immeuble.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ) $(TARGET) controller controller.o

.PHONY: all clean
