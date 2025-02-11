# Define the compiler and flags
CC = g++
CFLAGS = -g -I/usr/include/fastdds -I/usr/include/fastcdr -L/usr/local/lib -lfastdds -lfastcdr -lpthread
NCURSES_FLAGS = -lncurses

# Define the targets and their source files
all: server ObstaclesPublisher TargetsPublisher dynamic keyboard vis watchdog

server: server.cpp Generated/TargetsPubSubTypes.cxx Generated/TargetsTypeObjectSupport.cxx Generated/ObstaclesPubSubTypes.cxx Generated/ObstaclesTypeObjectSupport.cxx
	$(CC) $< Generated/TargetsPubSubTypes.cxx Generated/TargetsTypeObjectSupport.cxx Generated/ObstaclesPubSubTypes.cxx Generated/ObstaclesTypeObjectSupport.cxx -o $@ $(CFLAGS)

ObstaclesPublisher: ObstaclesPublisher.cpp Generated/ObstaclesPubSubTypes.cxx Generated/ObstaclesTypeObjectSupport.cxx
	$(CC) $< Generated/ObstaclesPubSubTypes.cxx Generated/ObstaclesTypeObjectSupport.cxx -o $@ $(CFLAGS)

TargetsPublisher: TargetsPublisher.cpp Generated/TargetsPubSubTypes.cxx Generated/TargetsTypeObjectSupport.cxx
	$(CC) $< Generated/TargetsPubSubTypes.cxx Generated/TargetsTypeObjectSupport.cxx -o $@ $(CFLAGS)

dynamic: dynamic.c
	$(CC) $< -o $@ $(CFLAGS)

keyboard: keyboard.c
	$(CC) $< -o $@ $(NCURSES_FLAGS)

vis: visualization.c
	$(CC) $< -o $@ $(NCURSES_FLAGS)

watchdog: watchdog.c
	$(CC) $< -o $@

# Clean up generated files
clean:
	rm -f server ObstaclesPublisher TargetsPublisher dynamic keyboard vis watchdog
	rm -f *.o