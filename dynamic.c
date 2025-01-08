/*=====================================*/
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <sys/file.h>
#include <time.h>
#include <stdarg.h>

#define TIMEOUT 8 // Timeout in seconds

#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_dynamic.txt"

/*======================================================*/
// API to update watchdog file with dynamic paramters
void update_watchdog_file();
/*======================================================*/
// API to clear content of log file
void clear_log_file(const char *filename);
/*======================================================*/
// API to appened content in log file
void append_to_log_file(const char *filename, const char *format, ...);
/*======================================================*/
/*=========================================================*/
#define PIPE_REQUEST_DRONE "./pipes/drone_Request"
#define PIPE_RESPONSE_DRONE "./pipes/drone_Respond"
/*=========================================================*/
/*=========================================================*/

#define MAX_TARGETS 10
#define MAX_OBSTACLES 20
/*=========================================================*/
/*=========================================================*/

typedef struct
{
    float M;      // Mass of the drone
    float K;      // Viscous coefficient
    float T;      // Time step (seconds)
    float F_max;  // Maximum force
    float X_max;  // Geofence X boundary
    float Y_max;  // Geofence Y boundary
    float rho;    // Obstacle perception radius
    float eta;    // Obstacle repulsion strength coefficient
    float zeta;   // Target attraction strength coefficient
    float d_goal; // Distance threshold for attraction
} Parameters;

// Drone state structure
typedef struct
{
    float x, y;   // Position
    float vx, vy; // Velocity
    float fx, fy; // Force
} Drone;

typedef struct
{
    int x, y;
    int value; // Value from 1 to 9
    int active;
    char action; // q (quit) or r (reload game), s (suspend game)
} Target;

typedef struct
{
    int x, y;
    int size;
    int active;
    char action; // q (quit) or r (reload game), s (suspend game)
} Obstacle;

typedef struct
{
    Drone drone;
    Target targets[MAX_TARGETS];
    Obstacle obstacles[MAX_OBSTACLES];
    char key_pressed[128];
    int score;
    int next_target;
    char message[128]; // Stores collision or collection messages
} SharedState;

/*=========================================================*/
/*=========================================================*/
/*=========================================================*/
void load_parameters(const char *filename, Parameters *params);
void update_Drone_dynamics(SharedState *state, float F_max, Parameters *param);
void print_shared_state(const SharedState *state);
/*=========================================================*/

/*=========================================================*/
/*=========================================================*/
Drone drone_tracker = {10, 10, 0, 0, 0, 0};

int main(void)
{
    clear_log_file(LOG_FILE_NAME);
    // Load simulation parameters
    Parameters params = {0};
    load_parameters("parameters.txt", &params);
    printf(" Parameters used : \n");
    append_to_log_file(LOG_FILE_NAME, "parameters used: \n");
    printf("M = %f \n", params.M);
    append_to_log_file(LOG_FILE_NAME, "M: %f\n", params.M);
    printf("K = %f \n", params.K);
    append_to_log_file(LOG_FILE_NAME, "K: %f\n", params.M);
    printf("T = %f \n", params.T);
    printf("F_MAX = %f \n", params.F_max);
    append_to_log_file(LOG_FILE_NAME, "F_MAX: %f\n", params.M);
    printf("X_MAX = %f \n", params.X_max);
    append_to_log_file(LOG_FILE_NAME, "X_MAX: %f\n", params.M);
    printf("Y_MAX = %f \n", params.Y_max);
    append_to_log_file(LOG_FILE_NAME, "Y_MAX: %f\n", params.M);

    // define the data to be sent to the server
    // intial positon request
    // open pipes of the drone request and response
    int fd_request_drone = open(PIPE_REQUEST_DRONE, O_WRONLY);
    printf("fd request opened successfuly\n");
    append_to_log_file(LOG_FILE_NAME, "fd request opened successfuly\n");
    int fd_response_drone = open(PIPE_RESPONSE_DRONE, O_RDONLY);
    printf("fd response opened successfuly\n");
    append_to_log_file(LOG_FILE_NAME, "fd response opened successfuly\n");
    // check weather pipes is opened successfully or not
    if (fd_request_drone == -1)
    {
        perror("Failed to open pipe request visualization");
        append_to_log_file(LOG_FILE_NAME, "failed to open pipe request visualizer\n");
        exit(1);
    }
    if (fd_response_drone == -1)
    {
        perror("Failed to open pipe response visualization");
        append_to_log_file(LOG_FILE_NAME, "failed to open pipe response visualizer\n");
        exit(1);
    }
    // define the struct that we will read data in from the server so we can deal with it here and then send request to the server with new position
    SharedState state;
    time_t last_logged_time = 0;
    while (1)
    {
        // Get user input and data of the server
        // send request to server
        write(fd_request_drone, &drone_tracker, sizeof(drone_tracker));
        // printf("================================================================\n");
        // printf("drone position sent succesffuly to the server x: (%f) y: (%f)\n", drone_tracker.x, drone_tracker.y);
        append_to_log_file(LOG_FILE_NAME, "drone position sent succesffuly to the server x: (%f) y: (%f)\n", drone_tracker.x, drone_tracker.y);
        // printf("================================================================\n");
        // waiting response
        read(fd_response_drone, &state, sizeof(state));
        append_to_log_file(LOG_FILE_NAME, "the data is receieved succesffuly from server\n");
        // printf("================================================================\n");
        // printf("pressed key is: %s\n", state.key_pressed);
        // printf("================================================================\n");

        /*==========================================================================*/
        // printing the whole struct to see that is happeping !
        // printf("====================1111111111111111111=========================\n");
        // printf("====================before update function=======================\n");
        // print_shared_state(&state);
        // printf("================================================================\n");

        // printf("====================2222222222222222222=========================\n");
        // printf("====================after update function========================\n");

        /*==========================================================================*/
        // now we have the data and ready for do our calculations to be sent to server again as request next time
        update_Drone_dynamics(&state, params.F_max, &params);
        append_to_log_file(LOG_FILE_NAME, "dorne dynamics updated correctly\n");
        // printf("====================22222222222222222222=========================\n");
        // printf("====================process_keyboard_input=========================\n");
        // print_shared_state(&state);

        // update drone_tracker;
        drone_tracker.x = state.drone.x;
        drone_tracker.y = state.drone.y;
        drone_tracker.vx = state.drone.vx;
        drone_tracker.vy = state.drone.vy;
        drone_tracker.fx = state.drone.fx;
        drone_tracker.fy = state.drone.fy;
        // so this will be the request to the server next time

        // sleep(0.3); // 0.1second
        // usleep((int)(params.T * 1000000));

        time_t current_time = time(NULL);
        if (current_time - last_logged_time >= TIMEOUT)
        { // Check if 3 seconds have passed
            update_watchdog_file();
            last_logged_time = current_time;
        }

        usleep((int)(100000));
    }
}

void print_shared_state(const SharedState *state)
{
    printf("Drone(x=%.2f, y=%.2f, vx=%.2f, vy=%.2f, fx=%.2f, fy=%.2f), \n",
           state->drone.x, state->drone.y, state->drone.vx, state->drone.vy,
           state->drone.fx, state->drone.fy);

    // printf("Targets=[");
    // for (int i = 0; i < MAX_TARGETS; i++) {
    //     printf("(x=%d, y=%d, value=%d, active=%d, action='%c')\n",
    //            state->targets[i].x, state->targets[i].y, state->targets[i].value,
    //            state->targets[i].active, state->targets[i].action);
    //     if (i < MAX_TARGETS - 1) printf(", ");
    // }
    // printf("], ");

    // printf("Obstacles=[");
    // for (int i = 0; i < MAX_OBSTACLES; i++) {
    //     printf("(x=%d, y=%d, size=%d, active=%d, action='%c')\n",
    //            state->obstacles[i].x, state->obstacles[i].y, state->obstacles[i].size,
    //            state->obstacles[i].active, state->obstacles[i].action);
    //     if (i < MAX_OBSTACLES - 1) printf(", ");
    // }
    // printf("], ");

    printf("KeyPressed='%s', \n", state->key_pressed);
    printf("Score=%d, \n", state->score);
    printf("Message='%s'\n", state->message);
}

/// Function to load simulation parameters from a file
void load_parameters(const char *filename, Parameters *params)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        perror("Failed to open parameters file");
        exit(1);
    }

    char line[128];
    while (fgets(line, sizeof(line), file))
    {
        // Skip comment lines
        if (line[0] == '#' || line[0] == '\n')
        {
            continue;
        }

        // Parse key-value pairs
        char key[32];
        float value;
        if (sscanf(line, "%s %f", key, &value) == 2)
        {
            if (strcmp(key, "M") == 0)
                params->M = value;
            else if (strcmp(key, "K") == 0)
                params->K = value;
            else if (strcmp(key, "T") == 0)
                params->T = value;
            else if (strcmp(key, "F_max") == 0)
                params->F_max = value;
            else if (strcmp(key, "X_max") == 0)
                params->X_max = value;
            else if (strcmp(key, "Y_max") == 0)
                params->Y_max = value;
            else if (strcmp(key, "rho") == 0)
                params->rho = value;
            else if (strcmp(key, "eta") == 0)
                params->eta = value;
            else if (strcmp(key, "zeta") == 0)
                params->zeta = value;
            else if (strcmp(key, "d_goal") == 0)
                params->d_goal = value;
            else
            {
                printf("Unknown key: %s\n", key);
            }
        }
    }

    fclose(file);

    // // Debugging: Print loaded parameters
    // printf("Loaded Parameters:\n");
    // printf("M = %f\n", params->M);
    // printf("K = %f\n", params->K);
    // printf("T = %f\n", params->T);
    // printf("F_max = %f\n", params->F_max);
    // printf("X_max = %f\n", params->X_max);
    // printf("Y_max = %f\n", params->Y_max);
    // printf("rho = %f\n", params->rho);
    // printf("eta = %f\n", params->eta);
    // printf("zeta = %f\n", params->zeta);
    // printf("d_goal = %f\n", params->d_goal);
}

// Function to process keyboard inputs
void update_Drone_dynamics(SharedState *state, float F_max, Parameters *params)
{
    // intialize attraction force and repolsive force
    float F_rep_x = 0.0f, F_rep_y = 0.0f;
    float F_attr_x = 0.0f, F_attr_y = 0.0f;
    float total_fx;
    float total_fy;

    if (strcmp(state->key_pressed, "up") == 0)
    {
        state->drone.fy -= 1.0f; // Increment upward force
        if (state->drone.fy < -F_max)
            state->drone.fy = -F_max; // Enforce maximum force
    }
    else if (strcmp(state->key_pressed, "down") == 0)
    {
        state->drone.fy += 1.0f; // Increment downward force
        if (state->drone.fy > F_max)
            state->drone.fy = F_max; // Enforce maximum force
    }
    else if (strcmp(state->key_pressed, "left") == 0)
    {
        state->drone.fx -= 1.0f; // Increment leftward force
        if (state->drone.fx < -F_max)
            state->drone.fx = -F_max; // Enforce maximum force
    }
    else if (strcmp(state->key_pressed, "right") == 0)
    {
        state->drone.fx += 1.0f; // Increment rightward force
        if (state->drone.fx > F_max)
            state->drone.fx = F_max; // Enforce maximum force
    }
    else if (strcmp(state->key_pressed, "up-left") == 0)
    {
        state->drone.fy -= 0.7071f; // Increment upward-left force
        state->drone.fx -= 0.7071f;
        if (state->drone.fy < -F_max)
            state->drone.fy = -F_max;
        if (state->drone.fx < -F_max)
            state->drone.fx = -F_max;
    }
    else if (strcmp(state->key_pressed, "up-right") == 0)
    {
        state->drone.fy -= 0.7071f; // Increment upward-right force
        state->drone.fx += 0.7071f;
        if (state->drone.fy < -F_max)
            state->drone.fy = -F_max;
        if (state->drone.fx > F_max)
            state->drone.fx = F_max;
    }
    else if (strcmp(state->key_pressed, "down-left") == 0)
    {
        state->drone.fy += 0.7071f; // Increment downward-left force
        state->drone.fx -= 0.7071f;
        if (state->drone.fy > F_max)
            state->drone.fy = F_max;
        if (state->drone.fx < -F_max)
            state->drone.fx = -F_max;
    }
    else if (strcmp(state->key_pressed, "down-right") == 0)
    {
        state->drone.fy += 0.7071f; // Increment downward-right force
        state->drone.fx += 0.7071f;
        if (state->drone.fy > F_max)
            state->drone.fy = F_max;
        if (state->drone.fx > F_max)
            state->drone.fx = F_max;
    }
    else if (strcmp(state->key_pressed, "stop") == 0)
    {
        state->drone.fx = 0.0f; // Stop all forces
        state->drone.fy = 0.0f;
        state->drone.vx = 0;
        state->drone.vy = 0;
    }
    else if (strcmp(state->key_pressed, "reset") == 0)
    {

        state->drone.x = 10.0f; // put in initial x position
        state->drone.y = 10.0f; // put in initial y position
        state->drone.fx = 0.0f; // Stop all forces
        state->drone.fy = 0.0f;
        state->drone.vx = 0;
        state->drone.vy = 0;
    }

    // Repulsive forces from obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        if (state->obstacles[i].active)
        {
            float dx = state->drone.x - state->obstacles[i].x;
            float dy = state->drone.y - state->obstacles[i].y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < params->rho)
            {
                float F_rep = params->eta * (1.0f / distance - 1.0f / params->rho) / (distance * distance);
                F_rep_x += F_rep * (dx / distance);
                F_rep_y += F_rep * (dy / distance);
            }
        }
    }
    // Geofence repulsion
    // X boundaries
    if (state->drone.x < params->rho)
    {
        float distance = state->drone.x; // Distance to left boundary
        float F_rep = params->eta * (1.0f / distance - 1.0f / params->rho) / (distance * distance);
        F_rep_x += F_rep; // Push right
    }
    if (params->X_max - state->drone.x < params->rho)
    {
        float distance = params->X_max - state->drone.x; // Distance to right boundary
        float F_rep = params->eta * (1.0f / distance - 1.0f / params->rho) / (distance * distance);
        F_rep_x -= F_rep; // Push left
    }

    // Y boundaries
    if (state->drone.y < params->rho)
    {
        float distance = state->drone.y; // Distance to bottom boundary
        float F_rep = params->eta * (1.0f / distance - 1.0f / params->rho) / (distance * distance);
        F_rep_y += F_rep; // Push up
    }
    if (params->Y_max - state->drone.y < params->rho)
    {
        float distance = params->Y_max - state->drone.y; // Distance to top boundary
        float F_rep = params->eta * (1.0f / distance - 1.0f / params->rho) / (distance * distance);
        F_rep_y -= F_rep; // Push down
    }

    total_fx = F_rep_x + state->drone.fx;
    total_fy = F_rep_y + state->drone.fy;
    // else if (strcmp(state->key_pressed, "default") == 0)
    // {
    //     state->drone.fx = state->drone.fx; // same previos force
    //     state->drone.fy = state->drone.fx;
    // }

    // Calculate acceleration

    float ax = (total_fx - params->K * state->drone.vx) / params->M;
    float ay = (total_fy - params->K * state->drone.vy) / params->M;

    // printf("=======================================================\n");
    // printf("ax is: %f ay is: %f\n", ax, ay);
    // printf("=======================================================\n");
    // // Update velocity
    state->drone.vx += (ax * params->T);
    state->drone.vy += (ay * params->T);
    // printf("=======================================================\n");
    // printf("vx is: %f vy is: %f\n", state->drone.vx, state->drone.vy);
    // printf("parameter k is: %f \n", params->K);
    // printf("parameter T is: %f \n", params->T);
    // printf("parameter T is: %f \n", params->rho);
    // printf("=======================================================\n");
    // Update position
    state->drone.x += (state->drone.vx * params->T);
    state->drone.y += (state->drone.vy * params->T);

    // printf("=======================================================\n");
    // printf("KeyPressed='%s', \n", state->key_pressed);
    // printf("=======================================================\n");

    // printf("=======================================================\n");
    // printf("postion x is: %f position y is: %f\n", state->drone.x, state->drone.y);
    // printf("=======================================================\n");
    // printf("=======================================================\n");
    // printf("repulsion fx is: %f fy is: %f\n", F_rep_x, F_rep_y);
    // printf("fx is: %f fy is: %f\n", total_fx, total_fy);
    // printf("=======================================================\n");
}

void update_watchdog_file()
{
    FILE *file = fopen(LOG_FILE, "r+"); // Open for reading and writing
    if (!file)
    {
        file = fopen(LOG_FILE, "w"); // Create the file if it doesn't exist
        if (!file)
        {
            perror("Failed to open log file");
            return;
        }
    }

    // Lock the file to prevent race conditions
    int fd = fileno(file);
    flock(fd, LOCK_EX);

    pid_t pid = getpid();
    time_t current_time = time(NULL);
    char line[256];
    int found = 0;

    // Read and update the existing entry
    FILE *temp_file = tmpfile(); // Temporary file to store updated data
    if (!temp_file)
    {
        perror("Failed to create temporary file");
        flock(fd, LOCK_UN);
        fclose(file);
        return;
    }

    while (fgets(line, sizeof(line), file))
    {
        pid_t existing_pid;
        time_t existing_time;
        sscanf(line, "%d,%ld", &existing_pid, &existing_time);

        if (existing_pid == pid)
        {
            // Update the current process's entry
            fprintf(temp_file, "%d,%ld\n", pid, current_time);
            found = 1;
        }
        else
        {
            // Copy other processes' entries unchanged
            fputs(line, temp_file);
        }
    }

    // If the process's PID was not found, append a new entry
    if (!found)
    {
        fprintf(temp_file, "%d,%ld\n", pid, current_time);
    }

    // Rewind the temp file and original file, and copy contents back
    rewind(temp_file);
    freopen(LOG_FILE, "w", file);
    while (fgets(line, sizeof(line), temp_file))
    {
        fputs(line, file);
    }

    // Unlock the file and close everything
    fclose(temp_file);
    flock(fd, LOCK_UN);
    fclose(file);
}

// Function to clear the log file
void clear_log_file(const char *filename)
{
    FILE *file = fopen(filename, "w"); // Open file in write mode to clear content
    if (file == NULL)
    {
        perror("Error opening log file to clear");
        exit(EXIT_FAILURE);
    }
    fclose(file); // Close the file after clearing
}

// Function to append data to the log file
void append_to_log_file(const char *filename, const char *format, ...)
{
    int fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0)
    {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    // Acquire an exclusive lock
    if (flock(fd, LOCK_EX) < 0)
    {
        perror("Error locking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Open a file stream for writing
    FILE *file = fdopen(fd, "a");
    if (file == NULL)
    {
        perror("Error opening file stream");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Write to the log file
    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file); // This also unlocks the file and closes the file descriptor
}