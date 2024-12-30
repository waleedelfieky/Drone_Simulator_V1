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
#include <sys/file.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define TIMEOUT 4 // Timeout in seconds

#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_obsticale.txt"

void update_watchdog_file();
/*=====================================*/
#define MAX_OBSTACLES 30
/*=====================================*/
// initlization API
void init(void);
/*=====================================*/
// API to generate obsticales
void obsticale_generator(void);
/*=====================================*/
unsigned int sleep_wrapper(unsigned int seconds);
// API to clear content of log file
void clear_log_file(const char *filename);
/*======================================================*/
// API to appened content in log file
void append_to_log_file(const char *filename, const char *format, ...);
/*======================================================*/
/*=====================================*/

/*=========================================================*/
#define PIPE_REQUEST_OBSTICALE "./pipes/obsticale_Request"
#define PIPE_RESPONSE_OBSTICALE "./pipes/obsticale_Respond"
/*=========================================================*/

typedef struct
{
    int x, y;
    int size;
    int active;  // 1 = active, 0 = inactive
    char action; // q (quit) or r (reload game), s (suspend game)
} Obstacle;

/*=======================================*/
// global varibale array of generetated targets
Obstacle generated_obsticales[MAX_OBSTACLES];
/*=======================================*/

// define file descriptor varibales and pid varibale
int fd_obsticale_generator;
// define the path
char *obsticale_pipe_generator = "./pipes/obsticalegenerator";
/*=======================================*/

void signal_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        append_to_log_file(LOG_FILE_NAME, "the signal is received and osticales is being generated\n");
        // generate a new targets
        obsticale_generator();
        append_to_log_file(LOG_FILE_NAME, "the signal is received and osticales is generated successfuly\n");
        // now we want to send our array of struct to the server !
        // so we will make a normal request to the server
        write(fd_obsticale_generator, generated_obsticales, sizeof(generated_obsticales));
        append_to_log_file(LOG_FILE_NAME, "obsticales is send successfully to the server\n");
    }
}

int main(void)
{

    init();
    // open pipes of the obsticale request and response
    int fd_request_obsticale = open(PIPE_REQUEST_OBSTICALE, O_WRONLY);
    printf("fd request opened successfuly\n");
    append_to_log_file(LOG_FILE_NAME, "fd request opened successfully\n");
    int fd_response_obsticale = open(PIPE_RESPONSE_OBSTICALE, O_RDONLY);
    printf("fd response opened successfuly\n");
    append_to_log_file(LOG_FILE_NAME, "fd response opened successfully\n");
    // check weather pipes is opened successfully or not
    if (fd_request_obsticale == -1)
    {
        perror("Failed to open pipe request visualization");
        append_to_log_file(LOG_FILE_NAME, "failed to open the request pipe\n");
        exit(1);
    }
    if (fd_response_obsticale == -1)
    {
        perror("Failed to open pipe response visualization");
        append_to_log_file(LOG_FILE_NAME, "failed to open the response pipe\n");
        exit(1);
    }
    time_t last_logged_time = 0;

    while (1)
    {
        time_t current_time = time(NULL);

        if (current_time - last_logged_time >= TIMEOUT)
        { // Check if 3 seconds have passed
            update_watchdog_file();
            last_logged_time = current_time;
        }
        obsticale_generator();

        write(fd_request_obsticale, generated_obsticales, sizeof(generated_obsticales));
        // wait 6 seconds before generate new obsticales
        sleep_wrapper(6);
    }
}

unsigned int sleep_wrapper(unsigned int seconds)
{
    unsigned int remaining = seconds;
    while (remaining > 0)
    {
        remaining = sleep(remaining);
    }
    return remaining;
}

void obsticale_generator(void)
{
    srand(time(NULL)); // Seed for random numbers

    printf("Obstacle Generator Process Started...\n");

    printf("Generating obstacles...\n");
    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        // Generate odd numbers for x (1–79) and y (1–19)
        generated_obsticales[i].x = (rand() % 45) * 2 + 1; // Random odd x position (1–79)
        generated_obsticales[i].y = (rand() % 20) * 2 + 1; // Random odd y position (1–19)

        // Set obstacle properties
        generated_obsticales[i].size = 1;     // Size (1x1)
        generated_obsticales[i].active = 1;   // Mark as active
        generated_obsticales[i].action = ' '; // Placeholder for action

        // write(fd_obstacle, &o, sizeof(o));
        printf("x is: %d, y is: %d, size is: %d, active is: %d, action is: %d\n", generated_obsticales[i].x, generated_obsticales[i].y, generated_obsticales[i].size, generated_obsticales[i].active, generated_obsticales[i].action);
        append_to_log_file(LOG_FILE_NAME, "x is: %d, y is: %d, size is: %d, active is: %d, action is: %d\n", generated_obsticales[i].x, generated_obsticales[i].y, generated_obsticales[i].size, generated_obsticales[i].active, generated_obsticales[i].action);
    }
}

void init(void)
{
    // clear log file content
    clear_log_file(LOG_FILE_NAME);
    pid_t pid = getpid();
    // make the fifo
    // create the fifo but check if it's already exist
    if (access(obsticale_pipe_generator, F_OK) != 0)
    {
        if (mkfifo(obsticale_pipe_generator, 0666) == -1)
        {
            perror("Error creating target_pipe_generator FIFO");
            append_to_log_file(LOG_FILE_NAME, "Error creating target_pipe_generator FIFO\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("created success\n");
    // open the fifo in writing mood
    fd_obsticale_generator = open(obsticale_pipe_generator, O_WRONLY);
    printf("open sucess success\n");

    // read the pid and save it into our global varibale
    write(fd_obsticale_generator, &pid, sizeof(pid));
    printf("write sucess success\n");
    append_to_log_file(LOG_FILE_NAME, "the write process successed\n");
    // close the pipes
    // close(fd_obsticale_generator);
    // printf("close sucess success\n");
    // Register the signal handler for SIGUSR1
    if (signal(SIGUSR1, signal_handler) == SIG_ERR)
    {
        perror("Error registering signal handler");
        append_to_log_file(LOG_FILE_NAME, "Error registering signal handler\n");
        exit(EXIT_FAILURE);
    }
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