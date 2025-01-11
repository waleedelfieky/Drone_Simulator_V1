# Define the compiler and flags
CC = gcc
CFLAGS = -lm
NCURSES_FLAGS = -lncurses

# Define the targets and their source files
all: server dynamic obsticalegenerator targetgenerator keyboard vis watchdog

server: server.c
	$(CC) $< -o $@ $(CFLAGS)

dynamic: dynamic.c
	$(CC) $< -o $@ $(CFLAGS)

obsticalegenerator: obsticalegenerator.c
	$(CC) $< -o $@

targetgenerator: targetgenerator.c
	$(CC) $< -o $@

keyboard: keyboard.c
	$(CC) $< -o $@ $(NCURSES_FLAGS)

vis: visualization.c
	$(CC) $< -o $@ $(NCURSES_FLAGS)

watchdog: watchdog.c
	$(CC) $< -o $@

# Clean up generated files
clean:
	rm -f server dynamic obsticalegenerator targetgenerator keyboard vis watchdog
	rm -f *.o
