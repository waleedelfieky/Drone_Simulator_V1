/*=================================================*/
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
#include <time.h>
#define TIMEOUT 4 // Timeout in seconds




#define LOG_FILE "watchdog_log.txt"
void update_watchdog_file();
/*=================================================*/
#define MAX_TARGETS 10

typedef struct
{
    int x, y;
    int value;   // Number (1-9)
    int active;  // 1 = active, 0 = inactive
    char action; // q (quit) or r (reload game), s (suspend game)
} Target;

/*=============================================*/
void init(void);
void target_generator(void);
/*=============================================*/

// global varibale array of generetated targets
Target generated_targets[MAX_TARGETS];
/*=============================================*/
// Signal handler function
// define file descriptor varibales and pid varibale
int fd_target_generator;
// define the path

char *target_pipe_generator = "./pipes/targetgenerator";

void signal_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        // generate a new targets
        target_generator();
        // now we want to send our array of struct to the server !
        // so we will make a normal request to the server
        write(fd_target_generator, generated_targets, sizeof(generated_targets));
    }
}

int main(void)
{

    // target_generator();
    init();
    time_t last_logged_time = 0;

    while (1)
    {
        time_t current_time = time(NULL);
        if (current_time - last_logged_time >= TIMEOUT)
        { // Check if 3 seconds have passed
            update_watchdog_file();
            last_logged_time = current_time;
        }
        sleep(5);
        // pause(); // Wait for a signal
    }
}

void target_generator(void)
{
    srand(time(NULL)); // Seed for random numbers
    printf("Target Generator Process Started...\n");

    printf("Generating targets...\n");
    for (int i = 1; i < MAX_TARGETS; i++)
    {
        // Generate even numbers between 0 and 50 for x and y
        generated_targets[i].x = (rand() % 50) * 2; // Random even x position (0-50)
        generated_targets[i].y = (rand() % 11) * 2; // Random even y position (0-50)

        generated_targets[i].value = i;    // Target value (0-9)
        generated_targets[i].active = 1;   // Mark as active
        generated_targets[i].action = ' '; // Placeholder for action

        // Send the target to the server
        // write(fd_target, &t, sizeof(t));
        // printf("Generated targets: (%d ,%d), value: %d\n" t.x, t.y, t.value);
        printf("x is: %d, y is: %d, value is: %d, active is: %d, action is: %d\n", generated_targets[i].x, generated_targets[i].y, generated_targets[i].value, generated_targets[i].active, generated_targets[i].action);
    }
}

void init(void)
{
    pid_t pid = getpid();

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
    printf("created success\n");
    // open the fifo in writing mood
    fd_target_generator = open(target_pipe_generator, O_WRONLY);
    printf("open sucess success\n");
    // read the pid and save it into our global varibale
    write(fd_target_generator, &pid, sizeof(pid));
    // close the pipes
    printf("write sucess success\n");
    // close(fd_target_generator);
    // printf("close sucess success\n");

    // Register the signal handler for SIGUSR1
    if (signal(SIGUSR1, signal_handler) == SIG_ERR)
    {
        perror("Error registering signal handler");
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