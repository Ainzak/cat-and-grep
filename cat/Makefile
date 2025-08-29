CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -I.
SRC = ainz_cat.c
OBJ = $(SRC:.c=.o)
BUILD = ainz_cat

all: $(BUILD)

$(BUILD): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -rf $(OBJ) $(BUILD)