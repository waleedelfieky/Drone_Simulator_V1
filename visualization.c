/*======================================================*/
#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
/*======================================================*/

#define TIMEOUT 8 // Timeout in seconds
#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_visualizer.txt"
#define PIPE_REQUEST_VISUALIZATION "./pipes/visualizer_Request"
#define PIPE_RESPONSE_VISUALIZATION "./pipes/visualizer_Respond"
#define MAX_TARGETS 10
#define MAX_OBSTACLES 20

/*======================================================*/

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
    int attemp;
} SharedState;
/*======================================================*/

/*======================================================*/
// API to update watchdog file
void update_watchdog_file();
// API to clear content of log file
void clear_log_file(const char *filename);
/*======================================================*/
// API to appened content in log file
void append_to_log_file(const char *filename, const char *format, ...);
void display(WINDOW *win, SharedState *state);
/*======================================================================*/
int main()
{
    clear_log_file(LOG_FILE_NAME);
    char *request = "REQUEST";
    int fd_request_visualization = open(PIPE_REQUEST_VISUALIZATION, O_WRONLY);
    append_to_log_file(LOG_FILE_NAME, "the fd request of vis is successfully opened\n");
    int fd_response_visualization = open(PIPE_RESPONSE_VISUALIZATION, O_RDONLY);
    append_to_log_file(LOG_FILE_NAME, "the fd response of vis is successfully opened\n");

    if (fd_request_visualization == -1)
    {
        perror("Failed to open pipe request visualization");
        append_to_log_file(LOG_FILE_NAME, "Failed to open pipe request visualization\n");
        exit(1);
    }
    if (fd_response_visualization == -1)
    {
        perror("Failed to open pipe response visualization");
        append_to_log_file(LOG_FILE_NAME, "Failed to open pipe response visualization\n");
        exit(1);
    }

    SharedState state;

    // Initialize ncurses
    initscr();
    append_to_log_file(LOG_FILE_NAME, "initscr is successfully called\n");
    cbreak();
    append_to_log_file(LOG_FILE_NAME, "cbreak is successfully called\n");
    noecho();
    append_to_log_file(LOG_FILE_NAME, "noecho is successfully called\n");
    curs_set(0);
    append_to_log_file(LOG_FILE_NAME, "curs_set is successfully called\n");
    start_color();
    append_to_log_file(LOG_FILE_NAME, "start color is successfully called\n");

    // Initialize colors
    init_pair(1, COLOR_BLUE, COLOR_BLACK);  // Drone color
    init_pair(2, COLOR_GREEN, COLOR_BLACK); // Target color
    init_pair(3, COLOR_RED, COLOR_BLACK);   // Obstacle color

    printf("6\n");
    // Create the window
    int height, width;
    getmaxyx(stdscr, height, width);
    WINDOW *win = newwin(height, width, 0, 0);

    printf("7\n");
    printf("Visualization started and request is sent. Waiting for updates...\n");
    // now wait the response
    time_t last_logged_time = 0;
    
    append_to_log_file(LOG_FILE_NAME, "system now is ready to update visualizer frequently with new value froom the server it will send request and wait response and then draw and so on \n");
    while (1)
    {
        // request from server
        write(fd_request_visualization, request, strlen(request) + 1);
        // read from server
        read(fd_response_visualization, &state, sizeof(state));
        printf("Received update: Drone (%f, %f)\n", state.drone.x, state.drone.y);
        display(win, &state);
        refresh();
        wrefresh(win);
        // sleep(1); // Refresh rate: 0.5 second
        // usleep((int)10000);
        time_t current_time = time(NULL);
        if (current_time - last_logged_time >= TIMEOUT)
        { // Check if 3 seconds have passed
            update_watchdog_file();
            last_logged_time = current_time;
        }
        usleep((int)10000);
        // usleep((int)7000);
    }
    printf("Visualization process ending.\n");

    // Cleanup
    delwin(win);
    endwin();
    // close(fd_visualization);

    return 0;
}


void display(WINDOW *win, SharedState *state)
{
    werase(win);
    wrefresh(win);
    refresh();
    

    // Display drone (position received from the drone process)
    mvwaddch(win, state->drone.y + 2, state->drone.x + 1, '+' | A_BOLD | COLOR_PAIR(1));

    // Display targets
    for (int i = 0; i < MAX_TARGETS; i++)
    {
        if (state->targets[i].active)
        {
            mvwaddch(win, state->targets[i].y + 2, state->targets[i].x + 1, state->targets[i].value + '0' | A_BOLD | COLOR_PAIR(2));
        }
    }

    // Display obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        if (state->obstacles[i].active)
        {
            for (int dx = 0; dx < state->obstacles[i].size; dx++)
            {
                for (int dy = 0; dy < state->obstacles[i].size; dy++)
                {
                    mvwaddch(win, state->obstacles[i].y + dy + 2, state->obstacles[i].x + dx + 1, 'O' | A_BOLD | COLOR_PAIR(3));
                }
            }
        }
    }

    // Refresh the window
    box(win, 0, 0);
    wrefresh(win);
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