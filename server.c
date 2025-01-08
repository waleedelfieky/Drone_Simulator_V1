#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/select.h>
#include <limits.h>
#include <signal.h>
#include <math.h>
#include <sys/file.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_server.txt"
// #define LOG_FILE_NAME "log.txt"
void update_watchdog_file();

#define READWRITEPERMISSION 0666
#define DEFAULT_FDVALUE 0
#define pair 2
#define NUMBER_OF_CLIENTS 4

#define keyboard_pipe_index 0
#define visualizer_pipe_index 1
#define drone_pipe_index 2
#define obsticale_pipe_index 3

#define MAX_TARGETS 10
#define MAX_OBSTACLES 20
#define TIMEOUT 8 // Timeout in seconds

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

SharedState state = {0};
// Set the first target to be collected as 1
SharedState reset_state = {0};

Target target_generator_reader[MAX_TARGETS];
Obstacle obsticale_generator_reader[MAX_OBSTACLES];

const int NUMBER_OF_PIPES = NUMBER_OF_CLIENTS * 2;

int closed_pipe_flags[NUMBER_OF_CLIENTS] = {0};

struct PIPES_T
{
    char *request_path;
    char *response_path;
    int permission;
    int fd_request;
    int fd_response;
};

/*=========================================================*/
/*=========================================================*/
// global variables
pid_t pid_targt_generator;
pid_t pid_obsticale_generator;
// define file descriptors varibales
int fd_target_generator;
int fd_obsticale_generator;
// define the paths
char *target_pipe_generator = "./pipes/targetgenerator";
char *obsticale_pipe_generator = "./pipes/obsticalegenerator";
/*=========================================================*/
// global varibales contains the fifos info
struct PIPES_T keyboard_pipe = {"./pipes/keyboard_Request", "./pipes/keyboard_Respond", READWRITEPERMISSION, DEFAULT_FDVALUE, DEFAULT_FDVALUE};
struct PIPES_T visualizer_pipe = {"./pipes/visualizer_Request", "./pipes/visualizer_Respond", READWRITEPERMISSION, DEFAULT_FDVALUE, DEFAULT_FDVALUE};
struct PIPES_T drone_pipe = {"./pipes/drone_Request", "./pipes/drone_Respond", READWRITEPERMISSION, DEFAULT_FDVALUE, DEFAULT_FDVALUE};
struct PIPES_T obsticale_pipe = {"./pipes/obsticale_Request", "./pipes/obsticale_Respond", READWRITEPERMISSION, DEFAULT_FDVALUE, DEFAULT_FDVALUE};
struct PIPES_T *pipes_paths[NUMBER_OF_CLIENTS] = {&keyboard_pipe, &visualizer_pipe, &drone_pipe, &obsticale_pipe};

/*======================================================*/
// api create all fifo special files
void create_fifos(struct PIPES_T **pipes_paths, int number_of_clients, int permission);
/*======================================================*/
// API to open all fifos
void open_fifos(struct PIPES_T **pipes_paths, int number_of_clients);
/*======================================================*/
// the purpose of this function is to see which request is filled with data then if it's ready call the crosspoinding function
void select_monitor(struct PIPES_T **pipes_paths, int number_of_clients);
/*======================================================*/
int max_request_fd(struct PIPES_T **pipes_paths, int number_of_clients);
/*======================================================*/
void fill_read_fds(struct PIPES_T **pipes_paths, int number_of_clients, fd_set *readfds_l);
/*======================================================*/
void keyboard_pipe_Work(char *keyboard_input);
/*======================================================*/
void init(void);
/*======================================================*/
void send_target_signal();
/*======================================================*/
void send_obsticale_signal();
/*======================================================*/
// API to clear content of log file
void clear_log_file(const char *filename);
/*======================================================*/
// API to appened content in log file
void append_to_log_file(const char *filename, const char *format, ...);
/*======================================================*/
int main()
{
    init();
    // create the pipes for every client we create a pipe for request and a pipe for response
    create_fifos(pipes_paths, NUMBER_OF_CLIENTS, READWRITEPERMISSION);
    open_fifos(pipes_paths, NUMBER_OF_CLIENTS);
    select_monitor(pipes_paths, NUMBER_OF_CLIENTS);
    return 0;
}

/*==========================================*/
void init(void)
{
    clear_log_file(LOG_FILE_NAME);
    // assign default key as "stop"
    // strcpy(state.key_pressed,"stop");
    state.drone.x = 10;
    state.drone.y = 10;
    state.drone.fx = 0;
    state.drone.fy = 0;
    state.drone.vx = 0;
    state.drone.vy = 0;
    // 2 temporary pipes for target generator and obsticale generator

    // make the fifo
    // create the fifo but check if it's already exist
    if (access(target_pipe_generator, F_OK) != 0)
    {
        if (mkfifo(target_pipe_generator, 0666) == -1)
        {
            perror("Error creating target_pipe_generator FIFO");
            exit(EXIT_FAILURE);
        }
    }
    if (access(obsticale_pipe_generator, F_OK) != 0)
    {
        if (mkfifo(obsticale_pipe_generator, 0666) == -1)
        {
            perror("Error creating target_pipe_generator FIFO");
            exit(EXIT_FAILURE);
        }
    }
    printf("create both pipes success sucess success\n");

    // now we are ready to open both fifos and send and recieve data
    //  we will read the pid of the processes so open fifos as read mood
    fd_target_generator = open(target_pipe_generator, O_RDONLY);
    printf("open for target sucess success\n");
    append_to_log_file(LOG_FILE_NAME, "open for first target generaotor pipe successfully done\n");
    // read the pid and save it into our global varibale
    read(fd_target_generator, &pid_targt_generator, sizeof(pid_t));
    printf("read for target success sucess success\n");
    // print pid to make sure
    printf("the pid of target generation is: %d\n", pid_targt_generator);
    append_to_log_file(LOG_FILE_NAME, "read of tareget PID done succesffuly and pid is %d\n", pid_targt_generator);
    // close(fd_target_generator);
    // printf("close for target sucess success\n");

    fd_obsticale_generator = open(obsticale_pipe_generator, O_RDONLY);
    printf("open for obsticale sucess success\n");
    append_to_log_file(LOG_FILE_NAME, "successfuly open pipe of obsicale for first time generation\n");
    read(fd_obsticale_generator, &pid_obsticale_generator, sizeof(pid_t));
    printf("read for obsticale sucess success\n");
    printf("the pid of obsticale generation is: %d\n", pid_obsticale_generator);
    append_to_log_file(LOG_FILE_NAME, "read of obsricales PID is done succesffuly and pid is %d\n", pid_obsticale_generator);
    send_target_signal();
    send_obsticale_signal();
}
/*======================================================*/

void create_fifos(struct PIPES_T **pipes_paths, int number_of_clients, int permission)
{
    // array to make the fifos and return back the fd of each one of then to their struct
    for (int i = 0; i < number_of_clients; i++)
    {
        // Check if the request FIFO exists
        if (access(pipes_paths[i]->request_path, F_OK) != 0)
        {
            if (mkfifo(pipes_paths[i]->request_path, permission) == -1)
            {
                perror("Error creating request FIFO");
                append_to_log_file(LOG_FILE_NAME, "Error creating request FIFO for %s\n", pipes_paths[i]->request_path);
                exit(EXIT_FAILURE);
            }
        }

        // Check if the response FIFO exists
        if (access(pipes_paths[i]->response_path, F_OK) != 0)
        {
            if (mkfifo(pipes_paths[i]->response_path, permission) == -1)
            {
                perror("Error creating response FIFO");
                append_to_log_file(LOG_FILE_NAME, "Error creating request FIFO for %s\n", pipes_paths[i]->request_path);
                exit(EXIT_FAILURE);
            }
        }
    }
}
/*======================================================*/
/*======================================================*/
/*
take care the opposite side must open the file too so the channel is connected
otherwise the code will stack here until other side start to open the pipe it will be blocked
*/
void open_fifos(struct PIPES_T **pipes_paths, int number_of_clients)
{
    // open all fifos and assign the file descriptor for each opened fifo
    for (int i = 0; i < number_of_clients; i++)
    {
        pipes_paths[i]->fd_request = open(pipes_paths[i]->request_path, O_RDONLY);
        printf("Opened request FIFO for reading: %s\n", pipes_paths[i]->request_path);
        append_to_log_file(LOG_FILE_NAME, "Opened request FIFO for reading %s\n", pipes_paths[i]->request_path);

        pipes_paths[i]->fd_response = open(pipes_paths[i]->response_path, O_WRONLY);
        printf("Opened request FIFO for reading: %s\n", pipes_paths[i]->response_path);
        append_to_log_file(LOG_FILE_NAME, "Opened request FIFO for reading %s\n", pipes_paths[i]->response_path);
    }
}

/*======================================================*/
/*======================================================*/
// API to return back the max request fd to be input to the select
int max_request_fd(struct PIPES_T **pipes_paths, int number_of_clients)
{
    int max_fd = INT_MIN; // Start with the smallest possible integer

    for (int i = 0; i < number_of_clients; i++)
    {
        if (pipes_paths[i]->fd_request > max_fd)
        {
            max_fd = pipes_paths[i]->fd_request;
        }
    }

    return max_fd;
}

/*======================================================*/
/*======================================================*/
// API to fill readfds data structure
void fill_read_fds(struct PIPES_T **pipes_paths, int number_of_clients, fd_set *readfds_l)
{
    for (int i = 0; i < number_of_clients; i++)
    {
        FD_SET(pipes_paths[i]->fd_request, readfds_l);
    }
}
/*======================================================*/
/*======================================================*/

void select_monitor(struct PIPES_T **pipes_paths, int number_of_clients)
{
    // define variable to carry the request tasks
    fd_set readfds;
    int ready;
    // define timespan
    struct timeval timeout = {1, 0};
    // get the max request fd
    int maxfd = max_request_fd(pipes_paths, number_of_clients) + 1;

    // Shared state
    int cell[2] = {0, 0};          // Values in cells
    int cell_previous[2] = {0, 0}; // Track previous values for readers

    time_t last_logged_time = 0;

    state.next_target = 1; // Set the first target to be collected as 1

    while (1)
    {
        FD_ZERO(&readfds);
        // fill readfds with data
        fill_read_fds(pipes_paths, number_of_clients, &readfds);

        ready = select(maxfd, &readfds, NULL, NULL, &timeout);

        if (ready == -1)
        {
            perror("select error");
            break;
        }
        else if (ready == 0)
        {
            // printf("No data available, waiting...\n");
            continue;
        }
        // print the read data from pipes
        for (int i = 0; i < number_of_clients; i++)
        {
            if (FD_ISSET(pipes_paths[i]->fd_request, &readfds))
            {
                // we need to check what is the pipe that we will revieve data from now to see how we will deal with the receieved data
                if (i == keyboard_pipe_index)
                {
                    // get data
                    char buffer[256] = {0};
                    int bytes_read = read(pipes_paths[i]->fd_request, buffer, sizeof(buffer));
                    if (bytes_read == 0)
                    {
                        if (closed_pipe_flags[i] == 0)
                        {

                            printf("Writer closed the pipe for: %s\n", pipes_paths[i]->request_path);
                            append_to_log_file(LOG_FILE_NAME, "Writer closed the pipe for: %s\n", pipes_paths[i]->request_path);
                            closed_pipe_flags[i] = 1;
                            continue; // Skip further processing for this iteration
                        }
                        else
                        {
                            continue;
                        }
                    }
                    // response to the keyboard by the struct
                    write(pipes_paths[i]->fd_response, &state, sizeof(state));
                    printf("===========================================================\n");
                    printf("Drone -> Position: (%.2f, %.2f), Velocity: (%.2f, %.2f), Force: (%.2f, %.2f)\n"
                           "Score: %d\n"
                           "Message: %s\n",
                           state.drone.x, state.drone.y,
                           state.drone.vx, state.drone.vy,
                           state.drone.fx, state.drone.fy,
                           state.score,
                           state.message);
                    printf("===========================================================\n");
                    keyboard_pipe_Work(buffer);
                }

                else if (i == visualizer_pipe_index)
                {
                    // get data
                    char buffer[256] = {0};
                    int bytes_read = read(pipes_paths[i]->fd_request, buffer, sizeof(buffer));
                    if (bytes_read == 0)
                    {
                        if (closed_pipe_flags[i] == 0)
                        {
                            printf("Writer closed the pipe for: %s\n", pipes_paths[i]->request_path);
                            append_to_log_file(LOG_FILE_NAME, "Writer closed the pipe for: %s\n", pipes_paths[i]->request_path);
                            closed_pipe_flags[i] = 1;
                            continue; // Skip further processing for this iteration
                        }
                        else
                        {
                            continue;
                        }
                    }
                    // the work to be done

                    write(pipes_paths[i]->fd_response, &state, sizeof(state));
                    // fflush(stdout);

                    // interpret the data
                    // the keyboard send to us only 1 character
                }

                else if (i == drone_pipe_index)
                {

                    // if drone make request the request would carry the new posostion
                    // read the new position
                    //  get data directly to our global struct
                    int bytes_read = read(pipes_paths[i]->fd_request, &state.drone, sizeof(state.drone));
                    // check if done succesfully
                    // check condition rules
                    for (int i = 0; i < MAX_TARGETS; i++)
                    {
                        if (state.targets[i].active)
                        {
                            // Calculate distance to target
                            float dx = state.drone.x - state.targets[i].x;
                            float dy = state.drone.y - state.targets[i].y;
                            float distance = sqrt(dx * dx + dy * dy);

                            // Target collection threshold
                            if (distance < 1.0f)
                            { // 1m proximity
                                if (state.targets[i].value == state.next_target)
                                {
                                    // Correct target collected
                                    state.score += state.targets[i].value; // Add value to score
                                    state.next_target++;                   // Move to the next target in sequence (from 1 to 9)
                                    if (state.next_target > 9)
                                    {
                                        state.next_target = 1; // Reset to 1 if all targets have been collected
                                    }
                                }
                                else
                                {
                                    // Incorrect target collected (penalty)
                                    state.score -= state.targets[i].value; // Subtract value as penalty
                                }

                                // Mark the target as inactive once collected
                                state.targets[i].active = 0;
                            }
                        }
                    }

                    // print closed in case of pipe closing
                    if (bytes_read == 0)
                    {
                        if (closed_pipe_flags[i] == 0)
                        {
                            printf("Writer closed the pipe for: %s\n", pipes_paths[i]->request_path);
                            append_to_log_file(LOG_FILE_NAME, "Writer closed the pipe for: %s\n", pipes_paths[i]->request_path);
                            closed_pipe_flags[i] = 1;
                            continue; // Skip further processing for this iteration
                        }
                        else
                        {
                            continue;
                        }
                    }
                    // respond to the drone with the new struct
                    write(pipes_paths[i]->fd_response, &state, sizeof(state));
                    strcpy(state.key_pressed, "default");
                }
                else if (i == obsticale_pipe_index)
                {
                    // get obsticale request
                    read(pipes_paths[i]->fd_request, obsticale_generator_reader, sizeof(obsticale_generator_reader));
                    // update it in our main struct
                    memcpy(state.obstacles, obsticale_generator_reader, sizeof(obsticale_generator_reader));
                }
            }

            // update watchdog file;
            time_t current_time = time(NULL);
            if (current_time - last_logged_time >= TIMEOUT)
            { // Check if 3 seconds have passed
                update_watchdog_file();
                last_logged_time = current_time;
            }
        }
    }
}

void keyboard_pipe_Work(char *keyboard_input)
{
    if (strcmp(keyboard_input, "up") == 0)
    {
        printf("Move up\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server is move up\n");
        // Add your action for "up"
        strcpy(state.key_pressed, "up");
    }
    else if (strcmp(keyboard_input, "down") == 0)
    {
        printf("Move down\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server is move down\n");
        // Add your action for "down"
        strcpy(state.key_pressed, "down");
    }
    else if (strcmp(keyboard_input, "left") == 0)
    {
        printf("Move left\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server is move left\n");
        // Add your action for "left"
        strcpy(state.key_pressed, "left");
    }
    else if (strcmp(keyboard_input, "right") == 0)
    {
        printf("Move right\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server is move right\n");
        strcpy(state.key_pressed, "right");
        // Add your action for "right"
    }
    else if (strcmp(keyboard_input, "stop") == 0)
    {
        printf("Stop movement\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server is stopt\n");
        // Add your action for "stop"
        strcpy(state.key_pressed, "stop");
    }
    else if (strcmp(keyboard_input, "up-left") == 0)
    {
        printf("Move up-left\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server move up left\n");
        // Add your action for "up-left"
        strcpy(state.key_pressed, "up-left");
    }
    else if (strcmp(keyboard_input, "up-right") == 0)
    {
        printf("Move up-right\n");
        // Add your action for "up-right"
        strcpy(state.key_pressed, "up-right");
        append_to_log_file(LOG_FILE_NAME, "key command on server move up right\n");
    }
    else if (strcmp(keyboard_input, "down-left") == 0)
    {
        printf("Move down-left\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server down left\n");
        // Add your action for "down-left"
        strcpy(state.key_pressed, "down-left");
    }
    else if (strcmp(keyboard_input, "down-right") == 0)
    {
        printf("Move down-right\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server down right\n");
        // Add your action for "down-right"
        strcpy(state.key_pressed, "down-right");
    }
    else if (strcmp(keyboard_input, "target") == 0)
    {
        send_target_signal();
        printf("===============================\n");
        printf("send target singal sent successfully\n");
        append_to_log_file(LOG_FILE_NAME, "send target singal sent successfully and waiting generation\n");
        printf("===============================\n");
    }
    else if (strcmp(keyboard_input, "obsticale") == 0)
    {
        send_obsticale_signal();
        append_to_log_file(LOG_FILE_NAME, "send obsticale singal sent successfully and waiting generation\n");
    }

    else if (strcmp(keyboard_input, "reset") == 0)
    {
        append_to_log_file(LOG_FILE_NAME, "reset signal is recevied and now server is process it\n");
        printf("Move down-right\n");
        append_to_log_file(LOG_FILE_NAME, "key command on server down right\n");
        // Add your action for "down-right"
        strcpy(state.key_pressed, "reset");

        // send signals to generat new targets and obsticales
        send_target_signal();
        append_to_log_file(LOG_FILE_NAME, "targets generated successfuly\n");
        send_obsticale_signal();
        append_to_log_file(LOG_FILE_NAME, "obsticales generated successfuly\n");
    }
    else if(strcmp(keyboard_input, "request_data")==0){
        printf("struct is sent successfully to keyboard\n");
    }
    else
    {
        printf("Unknown input: %s\n", keyboard_input);
        append_to_log_file(LOG_FILE_NAME, "server recieved unkonwn input from keyboard: %s\n", keyboard_input);
        // Handle unknown input
    }
}

void send_target_signal()
{
    // send signal to the target generator pipe
    if (kill(pid_targt_generator, SIGUSR1) == -1)
    {
        perror("Failed to send SIGUSR1");
        append_to_log_file(LOG_FILE_NAME, "Failed to send SIGUSR1\n");
        exit(EXIT_FAILURE);
    }
    printf("SIGUSR1 sent to process %d.\n", pid_targt_generator);
    append_to_log_file(LOG_FILE_NAME, "SIGUSR1 sent to process %d.\n", pid_targt_generator);
    // sleep(3);
    read(fd_target_generator, target_generator_reader, sizeof(target_generator_reader));
    // output to test
    for (int i = 1; i < MAX_TARGETS; i++)
    {
        printf("x is: %d, y is: %d, value is: %d, active is: %d, action is: %d\n", target_generator_reader[i].x, target_generator_reader[i].y, target_generator_reader[i].value, target_generator_reader[i].active, target_generator_reader[i].action);
        append_to_log_file(LOG_FILE_NAME, "server received targets for first time is obsticale number [%d] x is: %d, y is: %d, value is: %d, active is: %d, action is: %d\n", i, target_generator_reader[i].x, target_generator_reader[i].y, target_generator_reader[i].value, target_generator_reader[i].active, target_generator_reader[i].action);
    }
    // upadte in global varibale
    memcpy(state.targets, target_generator_reader, sizeof(target_generator_reader));
}

void send_obsticale_signal()
{
    // send signal to the obsticale generator pipe
    if (kill(pid_obsticale_generator, SIGUSR1) == -1)
    {
        perror("Failed to send SIGUSR1");
        append_to_log_file(LOG_FILE_NAME, "Failed to send SIGUSR1\n");
        exit(EXIT_FAILURE);
    }
    printf("SIGUSR1 sent to process %d.\n", pid_obsticale_generator);
    append_to_log_file(LOG_FILE_NAME, "SIGUSR1 sent to process %d.\n", pid_obsticale_generator);
    // sleep(3);
    read(fd_obsticale_generator, obsticale_generator_reader, sizeof(obsticale_generator_reader));
    // output to test
    for (int i = 1; i < MAX_TARGETS; i++)
    {
        printf("x is: %d, y is: %d, value is: %d, active is: %d, action is: %d\n", obsticale_generator_reader[i].x, obsticale_generator_reader[i].y, obsticale_generator_reader[i].size, obsticale_generator_reader[i].active, obsticale_generator_reader[i].action);
        append_to_log_file(LOG_FILE_NAME, "obsticle received for first time is number [ %d] x is: %d, y is: %d, value is: %d, active is: %d, action is: %d\n", i, obsticale_generator_reader[i].x, obsticale_generator_reader[i].y, obsticale_generator_reader[i].size, obsticale_generator_reader[i].active, obsticale_generator_reader[i].action);
    }
    memcpy(state.obstacles, obsticale_generator_reader, sizeof(obsticale_generator_reader));
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
            append_to_log_file(LOG_FILE_NAME, "Failed to open log file\n");

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
        append_to_log_file(LOG_FILE_NAME, "Failed to create temporary file\n");
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