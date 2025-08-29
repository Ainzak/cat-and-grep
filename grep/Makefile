CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L -I.
SRC = ainz_grep.c
OBJ = $(SRC:.c=.o)
BUILD = ainz_grep

all: $(BUILD)

$(BUILD): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

clean:
	rm -rf $(OBJ) $(BUILD)