#!/bin/bash

# List of files to create
files=(
    "log_server.txt"
    "log_obsticale.txt"
    "log_keyboard.txt"
    "log_dynamic.txt"
    "log_visualizer.txt"
    "log_target.txt"
)

# Loop through the list and create/overwrite each file
for file in "${files[@]}"; do
    > "$file"
    echo "Created/overwritten: $file"
done

echo "All files have been created/overwritten."


# Function to handle termination signals
cleanup() {
    echo "Terminating all background processes..."
    trap - SIGINT SIGTERM  # Disable trap to prevent recursion
    kill 0  # Kill all processes in the current process group
    exit 0
}

# Trap SIGINT (Ctrl + C) and SIGTERM (Terminal close)
trap cleanup SIGINT SIGTERM


# Paths to pipes
PIPE_DIR="./pipes"
KEYBOARD_REQUEST_PIPE="$PIPE_DIR/keyboard_Request"
KEYBOARD_RESPONSE_PIPE="$PIPE_DIR/keyboard_Respond"
VISUALIZER_REQUEST_PIPE="$PIPE_DIR/visualizer_Request"
VISUALIZER_RESPONSE_PIPE="$PIPE_DIR/visualizer_Respond"
DRONE_REQUEST_PIPE="$PIPE_DIR/drone_Request"
DRONE_RESPONSE_PIPE="$PIPE_DIR/drone_Respond"
OBSTACLE_REQUEST_PIPE="$PIPE_DIR/obsticale_Request"
OBSTACLE_RESPONSE_PIPE="$PIPE_DIR/obsticale_Respond"
TARGET_PIPE_GENERATOR="$PIPE_DIR/targetgenerator"
OBSTACLE_PIPE_GENERATOR="$PIPE_DIR/obsticalegenerator"

# Create the pipes directory if it doesn't exist
mkdir -p $PIPE_DIR

# Function to create a pipe
create_pipe() {
    if [[ ! -p $1 ]]; then
        mkfifo $1
        echo "Created pipe: $1"
    else
        echo "Pipe already exists: $1"
    fi
}

# Create all required pipes
create_pipe $KEYBOARD_REQUEST_PIPE
create_pipe $KEYBOARD_RESPONSE_PIPE
create_pipe $VISUALIZER_REQUEST_PIPE
create_pipe $VISUALIZER_RESPONSE_PIPE
create_pipe $DRONE_REQUEST_PIPE
create_pipe $DRONE_RESPONSE_PIPE
create_pipe $OBSTACLE_REQUEST_PIPE
create_pipe $OBSTACLE_RESPONSE_PIPE
create_pipe $TARGET_PIPE_GENERATOR
create_pipe $OBSTACLE_PIPE_GENERATOR


# Compilation Script
gcc server.c -o server -lm
sleep 0.2
gcc dynamic.c -o dynamic -lm
sleep 0.2
gcc obsticalegenerator.c -o obsticalegenerator
sleep 0.2
gcc targetgenerator.c -o targetgenerator
sleep 0.2
gcc keyboard.c -o keyboard -lncurses
sleep 0.2
gcc visualization.c -o vis -lncurses
sleep 0.2
gcc watchdog.c -o watchdog

echo "Compilation complete!"


# Start background processes
./server &
./dynamic &
./obsticalegenerator &
./targetgenerator &

# Start Konsole processes
konsole --qwindowgeometry 922x700 -e ./keyboard &
konsole --qwindowgeometry 1000x1000 -e ./vis &
konsole --qwindowgeometry 900x300 -e ./watchdog &

# Wait for all background processes to complete
wait
