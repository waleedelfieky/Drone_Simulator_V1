#!/bin/bash

# Ask the user which mode they want to start
echo "Which mode do you want to start?"
echo "1 - Build and run ObstaclesPublisher and TargetsPublisher only"
echo "2 - Build the project, create pipes, and run the whole game (excluding ObstaclesPublisher and TargetsPublisher)"
read -p "Enter your choice (1 or 2): " mode

# Validate the user's input
if [[ "$mode" != "1" && "$mode" != "2" ]]; then
    echo "Invalid choice. Exiting."
    exit 1
fi

# Function to handle termination signals
cleanup() {
    echo "Terminating all background processes..."
    trap - SIGINT SIGTERM  # Disable trap to prevent recursion
    kill 0  # Kill all processes in the current process group
    exit 0
}

# Trap SIGINT (Ctrl + C) and SIGTERM (Terminal close)
trap cleanup SIGINT SIGTERM

# Clean the project (optional, if Makefile has a clean target)
echo "Cleaning the project..."
make clean

# Build the project based on the user's choice
if [[ "$mode" == "1" ]]; then
    echo "Building ObstaclesPublisher and TargetsPublisher..."
    make
    if [[ $? -ne 0 ]]; then
        echo "Build failed. Exiting."
        exit 1
    fi
    echo "Build complete!"

    echo "Starting ObstaclesPublisher and TargetsPublisher..."
    ./ObstaclesPublisher &
    ./TargetsPublisher &
    konsole --qwindowgeometry 900x300 -e ./watchdog &
else
    echo "Building the entire project..."
    make
    if [[ $? -ne 0 ]]; then
        echo "Build failed. Exiting."
        exit 1
    fi
    echo "Build complete!"

    # Paths to pipes
    PIPE_DIR="./pipes"
    KEYBOARD_REQUEST_PIPE="$PIPE_DIR/keyboard_Request"
    KEYBOARD_RESPONSE_PIPE="$PIPE_DIR/keyboard_Respond"
    VISUALIZER_REQUEST_PIPE="$PIPE_DIR/visualizer_Request"
    VISUALIZER_RESPONSE_PIPE="$PIPE_DIR/visualizer_Respond"
    DRONE_REQUEST_PIPE="$PIPE_DIR/drone_Request"
    DRONE_RESPONSE_PIPE="$PIPE_DIR/drone_Respond"

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

    # Start background processes
    ./server &
    ./dynamic &

    # Start Konsole processes
    konsole --qwindowgeometry 1000x1000 -e ./vis &
    sleep 1   # Delay of 1 second
    konsole --qwindowgeometry 900x300 -e ./watchdog &
    sleep 1   # Delay of 1 second
    konsole --qwindowgeometry 922x700 -e ./keyboard &
fi

# Wait for all background processes to complete
wait
