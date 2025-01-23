//======================================================
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
//======================================================

//======================================================
//Define Macros
#define TIMEOUT 4 // Timeout in seconds
#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_target.txt"
#define MAX_TARGETS 10
//======================================================
// Data Structures
typedef struct {
    int x, y;
    int value;   // Number (1-9)
    int active;  // 1 = active, 0 = inactive
    char action; // q (quit) or r (reload game), s (suspend game)
} Target;
//======================================================

//======================================================
// Global Variables
Target generated_targets[MAX_TARGETS];
int fd_target_generator;
char *target_pipe_generator = "./pipes/targetgenerator";
//======================================================

//======================================================
// Function Declarations
void update_watchdog_file();
void clear_log_file(const char *filename);
void append_to_log_file(const char *filename, const char *format, ...);
void init(void);
void target_generator(void);
void signal_handler(int signum);
//======================================================


// Main Function
int main(void) {
    clear_log_file(LOG_FILE_NAME);
    init();

    time_t last_logged_time = 0;

    while (1) {
        time_t current_time = time(NULL);
        if (current_time - last_logged_time >= TIMEOUT) {
            update_watchdog_file();
            last_logged_time = current_time;
        }
        sleep(5); // Wait for 5 seconds
    }

    return 0;
}


// Signal Handler Function
void signal_handler(int signum) {
    if (signum == SIGUSR1) {
        append_to_log_file(LOG_FILE_NAME, "the signal is received successfully and the target is being generated\n");
        target_generator();
        write(fd_target_generator, generated_targets, sizeof(generated_targets));
    }
}

// Target Generator Function
void target_generator(void) {
    srand(time(NULL)); // Seed for random numbers
    printf("Target Generator Process Started...\n");
    append_to_log_file(LOG_FILE_NAME, "Target Generator Process Started...\n");

    printf("Generating targets...\n");
    append_to_log_file(LOG_FILE_NAME, "Generating targets...\n");

    for (int i = 1; i < MAX_TARGETS; i++) {
        // Generate even numbers between 0 and 50 for x and y
        generated_targets[i].x = (rand() % 50) * 2; // Random even x position (0-50)
        generated_targets[i].y = (rand() % 11) * 2; // Random even y position (0-50)

        generated_targets[i].value = i;    // Target value (0-9)
        generated_targets[i].active = 1;   // Mark as active
        generated_targets[i].action = ' '; // Placeholder for action

        printf("x is: %d, y is: %d, value is: %d, active is: %d, action is: %d\n",
               generated_targets[i].x, generated_targets[i].y,
               generated_targets[i].value, generated_targets[i].active, generated_targets[i].action);

        append_to_log_file(LOG_FILE_NAME, "x is: %d, y is: %d, value is: %d, active is: %d, action is: %d\n",
                          generated_targets[i].x, generated_targets[i].y,
                          generated_targets[i].value, generated_targets[i].active, generated_targets[i].action);
    }
}

// Initialization Function
void init(void) {
    pid_t pid = getpid();

    // Create the FIFO if it doesn't exist
    if (access(target_pipe_generator, F_OK) != 0) {
        if (mkfifo(target_pipe_generator, 0666) == -1) {
            perror("Error creating target_pipe_generator FIFO");
            append_to_log_file(LOG_FILE_NAME, "Error creating target_pipe_generator FIFO\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("created success\n");
    append_to_log_file(LOG_FILE_NAME, "Target Generator pipe is created successfully...\n");

    // Open the FIFO in write mode
    fd_target_generator = open(target_pipe_generator, O_WRONLY);
    printf("open success\n");
    append_to_log_file(LOG_FILE_NAME, "Target Generator pipe is opened successfully...\n");

    // Write the PID to the pipe
    write(fd_target_generator, &pid, sizeof(pid));
    append_to_log_file(LOG_FILE_NAME, "PID Target Generator process is sent successfully to the server...\n");

    printf("write success\n");

    // Register the signal handler for SIGUSR1
    if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
        perror("Error registering signal handler");
        exit(EXIT_FAILURE);
    }
}

// Watchdog File Update Function
void update_watchdog_file() {
    FILE *file = fopen(LOG_FILE, "r+");
    if (!file) {
        file = fopen(LOG_FILE, "w");
        if (!file) {
            perror("Failed to open log file");
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