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
#include <stdarg.h>

// Constants and Macros
#define TIMEOUT 4 // Timeout in seconds
#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_obsticale.txt"
#define MAX_OBSTACLES 30
#define PIPE_REQUEST_OBSTICALE "./pipes/obsticale_Request"
#define PIPE_RESPONSE_OBSTICALE "./pipes/obsticale_Respond"

// Data Structures
typedef struct {
    int x, y;
    int size;
    int active;  // 1 = active, 0 = inactive
    char action; // q (quit) or r (reload game), s (suspend game)
} Obstacle;

// Global Variables
Obstacle generated_obsticales[MAX_OBSTACLES];
int fd_obsticale_generator;
char *obsticale_pipe_generator = "./pipes/obsticalegenerator";

// Function Declarations
void update_watchdog_file();
void init(void);
void obsticale_generator(void);
unsigned int sleep_wrapper(unsigned int seconds);
void clear_log_file(const char *filename);
void append_to_log_file(const char *filename, const char *format, ...);
void signal_handler(int signum);

// Main Function
int main(void) {
    init();

    // Open pipes for obsticale request and response
    int fd_request_obsticale = open(PIPE_REQUEST_OBSTICALE, O_WRONLY);
    printf("fd request opened successfully\n");
    append_to_log_file(LOG_FILE_NAME, "fd request opened successfully\n");

    int fd_response_obsticale = open(PIPE_RESPONSE_OBSTICALE, O_RDONLY);
    printf("fd response opened successfully\n");
    append_to_log_file(LOG_FILE_NAME, "fd response opened successfully\n");

    // Check if pipes were opened successfully
    if (fd_request_obsticale == -1) {
        perror("Failed to open pipe request visualization");
        append_to_log_file(LOG_FILE_NAME, "failed to open the request pipe\n");
        exit(1);
    }
    if (fd_response_obsticale == -1) {
        perror("Failed to open pipe response visualization");
        append_to_log_file(LOG_FILE_NAME, "failed to open the response pipe\n");
        exit(1);
    }

    time_t last_logged_time = 0;

    while (1) {
        time_t current_time = time(NULL);

        if (current_time - last_logged_time >= TIMEOUT) {
            update_watchdog_file();
            last_logged_time = current_time;
        }

        obsticale_generator();
        write(fd_request_obsticale, generated_obsticales, sizeof(generated_obsticales));
        sleep_wrapper(6); // Wait 6 seconds before generating new obstacles
    }

    return 0;
}

// Signal Handler
void signal_handler(int signum) {
    if (signum == SIGUSR1) {
        append_to_log_file(LOG_FILE_NAME, "the signal is received and obstacles are being generated\n");
        obsticale_generator();
        append_to_log_file(LOG_FILE_NAME, "the signal is received and obstacles are generated successfully\n");
        write(fd_obsticale_generator, generated_obsticales, sizeof(generated_obsticales));
        append_to_log_file(LOG_FILE_NAME, "obstacles are sent successfully to the server\n");
    }
}

// Initialization Function
void init(void) {
    clear_log_file(LOG_FILE_NAME);
    pid_t pid = getpid();

    // Create the FIFO if it doesn't exist
    if (access(obsticale_pipe_generator, F_OK) != 0) {
        if (mkfifo(obsticale_pipe_generator, 0666) == -1) {
            perror("Error creating target_pipe_generator FIFO");
            append_to_log_file(LOG_FILE_NAME, "Error creating target_pipe_generator FIFO\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("created success\n");
    fd_obsticale_generator = open(obsticale_pipe_generator, O_WRONLY);
    printf("open success success\n");

    // Write the PID to the pipe
    write(fd_obsticale_generator, &pid, sizeof(pid));
    printf("write success success\n");
    append_to_log_file(LOG_FILE_NAME, "the write process succeeded\n");

    // Register the signal handler for SIGUSR1
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        perror("Error registering signal handler");
        append_to_log_file(LOG_FILE_NAME, "Error registering signal handler\n");
        exit(EXIT_FAILURE);
    }
}

// Obstacle Generator Function
void obsticale_generator(void) {
    srand(time(NULL)); // Seed for random numbers

    printf("Obstacle Generator Process Started...\n");
    printf("Generating obstacles...\n");

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        // Generate odd numbers for x (1–79) and y (1–19)
        generated_obsticales[i].x = (rand() % 45) * 2 + 1; // Random odd x position (1–79)
        generated_obsticales[i].y = (rand() % 20) * 2 + 1; // Random odd y position (1–19)

        // Set obstacle properties
        generated_obsticales[i].size = 1;     // Size (1x1)
        generated_obsticales[i].active = 1;   // Mark as active
        generated_obsticales[i].action = ' '; // Placeholder for action

        printf("x is: %d, y is: %d, size is: %d, active is: %d, action is: %d\n",
               generated_obsticales[i].x, generated_obsticales[i].y,
               generated_obsticales[i].size, generated_obsticales[i].active, generated_obsticales[i].action);

        append_to_log_file(LOG_FILE_NAME, "x is: %d, y is: %d, size is: %d, active is: %d, action is: %d\n",
                          generated_obsticales[i].x, generated_obsticales[i].y,
                          generated_obsticales[i].size, generated_obsticales[i].active, generated_obsticales[i].action);
    }
}

// Sleep Wrapper Function
unsigned int sleep_wrapper(unsigned int seconds) {
    unsigned int remaining = seconds;
    while (remaining > 0) {
        remaining = sleep(remaining);
    }
    return remaining;
}

// Watchdog File Update Function
void update_watchdog_file() {
    FILE *file = fopen(LOG_FILE, "r+");
    if (!file) {
        file = fopen(LOG_FILE, "w");
        if (!file) {
            perror("Failed to open log file");
            append_to_log_file(LOG_FILE_NAME, "Failed to open log file\n");
            return;
        }
    }

    int fd = fileno(file);
    flock(fd, LOCK_EX);

    pid_t pid = getpid();
    time_t current_time = time(NULL);
    char line[256];
    int found = 0;

    FILE *temp_file = tmpfile();
    if (!temp_file) {
        perror("Failed to create temporary file");
        append_to_log_file(LOG_FILE_NAME, "Failed to create temporary file\n");
        flock(fd, LOCK_UN);
        fclose(file);
        return;
    }

    while (fgets(line, sizeof(line), file)) {
        pid_t existing_pid;
        time_t existing_time;
        sscanf(line, "%d,%ld", &existing_pid, &existing_time);

        if (existing_pid == pid) {
            fprintf(temp_file, "%d,%ld\n", pid, current_time);
            found = 1;
        } else {
            fputs(line, temp_file);
        }
    }

    if (!found) {
        fprintf(temp_file, "%d,%ld\n", pid, current_time);
    }

    rewind(temp_file);
    freopen(LOG_FILE, "w", file);
    while (fgets(line, sizeof(line), temp_file)) {
        fputs(line, file);
    }

    fclose(temp_file);
    flock(fd, LOCK_UN);
    fclose(file);
}

// Clear Log File Function
void clear_log_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening log file to clear");
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

// Append to Log File Function
void append_to_log_file(const char *filename, const char *format, ...) {
    int fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    if (flock(fd, LOCK_EX) < 0) {
        perror("Error locking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    FILE *file = fdopen(fd, "a");
    if (file == NULL) {
        perror("Error opening file stream");
        close(fd);
        exit(EXIT_FAILURE);
    }

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file);
}