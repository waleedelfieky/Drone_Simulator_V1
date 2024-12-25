#!/bin/bash

# Function to handle termination signals
cleanup() {
    echo "Terminating all background processes..."
    trap - SIGINT SIGTERM  # Disable trap to prevent recursion
    kill 0  # Kill all processes in the current process group
    exit 0
}

# Trap SIGINT (Ctrl + C) and SIGTERM (Terminal close)
trap cleanup SIGINT SIGTERM

# Start background processes
./server &
./drone &
./obsticalegenerator &
./targetgenerator &

# Start Konsole processes
konsole --qwindowgeometry 922x965 -e ./keyboard &
konsole --qwindowgeometry 1000x1000 -e ./vis &

# Wait for all background processes to complete
wait

