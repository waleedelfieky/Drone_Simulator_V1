//===========================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/file.h>
#include <dirent.h>
#include <string.h>
#include <signal.h>
//===========================================================

//===========================================================
#define LOG_FILE "watchdog_log.txt"
#define TEMP_FILE "temp_log.txt" // Ensure TEMP_FILE is properly defined
#define TIMEOUT 10 // Timeout in seconds
//===========================================================

//===========================================================
void clear_log_file();
void get_process_name(pid_t pid, char *process_name, size_t size);
void check_processes();
void terminate_all_processes();
//===========================================================


//===========================================================
const char *process_names[] = {"vis", "keyboard", "ObstaclesPublisher", "server", "TargetsPublisher", "dynamic"};
const int process_count = sizeof(process_names) / sizeof(process_names[0]);
//===========================================================


int main() {
    clear_log_file();
    while (1) {
        check_processes();
        sleep(1); // Check every second
    }

    return 0;
}


void clear_log_file() {
    FILE *file = fopen(LOG_FILE, "w"); // Open the file in write mode to truncate it
    if (file) {
        fclose(file); // Closing the file immediately clears its content
    } else {
        perror("Failed to clear log file");
    }
}

void get_process_name(pid_t pid, char *process_name, size_t size) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid); // Construct the path to the process's comm file

    FILE *file = fopen(path, "r");
    if (file) {
        if (fgets(process_name, size, file)) {
            // Remove the trailing newline character if present
            size_t len = strlen(process_name);
            if (len > 0 && process_name[len - 1] == '\n') {
                process_name[len - 1] = '\0';
            }
        } else {
            snprintf(process_name, size, "Unknown"); // Fallback if reading fails
        }
        fclose(file);
    } else {
        snprintf(process_name, size, "Unknown"); // Fallback if the file doesn't exist
    }
}
void check_processes() {
    FILE *file = fopen(LOG_FILE, "r");
    if (!file) {
        perror("Failed to open log file");
        return;
    }

    FILE *temp_file = fopen(TEMP_FILE, "w");
    if (!temp_file) {
        perror("Failed to open temporary file");
        fclose(file);
        return;
    }

    // Lock the file to prevent race conditions
    int fd = fileno(file);
    flock(fd, LOCK_EX);

    char line[256];
    time_t current_time = time(NULL);

    //printf("Current time: %ld\n", current_time); // Print the current time in seconds
    printf("=======================================================\n");
    while (fgets(line, sizeof(line), file)) {
        pid_t pid;
        time_t last_execution;

        // Parse the line for PID and last execution time
        if (sscanf(line, "%d,%ld", &pid, &last_execution) != 2) {
            fprintf(stderr, "Malformed line in log file: %s", line);
            fputs(line, temp_file); // Copy malformed lines to temp file
            continue;
        }

        // Get the process name
        char process_name[256];
        get_process_name(pid, process_name, sizeof(process_name));

        // Calculate the time difference
        time_t time_diff = current_time - last_execution;

        // Print debugging information
        printf("Process Name: %s, PID: %d, Last Execution Time: %ld, Time Diff: %ld seconds\n",
               process_name, pid, last_execution, time_diff);

        // Check if the process is unresponsive
        if (time_diff > TIMEOUT) {
            printf("Process %s (PID: %d) is unresponsive. Killing all processes now....\n", process_name, pid);
            //kill(pid, SIGKILL); // Kill the unresponsive process
            terminate_all_processes();
            exit(0);
            // Do not copy this line to the temp file (effectively removing it)
        } else {
            // Copy valid and responsive process lines to temp file
            fputs(line, temp_file);
        }
    }
    printf("=======================================================\n");

    // Unlock the file and close it
    flock(fd, LOCK_UN);
    fclose(file);
    fclose(temp_file);

    // Replace the original log file with the updated temp file
    if (rename(TEMP_FILE, LOG_FILE) != 0) {
        perror("Failed to update log file");
    }
}


// Function to terminate all processes with the specified names
void terminate_all_processes() {
    DIR *proc_dir = opendir("/proc");
    if (!proc_dir) {
        perror("Failed to open /proc directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(proc_dir)) != NULL) {
        // Check if the directory name is numeric (indicating a PID)
        char *endptr;
        pid_t pid = strtol(entry->d_name, &endptr, 10); // Convert directory name to PID
        if (*endptr == '\0') { // Successfully parsed a numeric PID
            char process_name[256];
            get_process_name(pid, process_name, sizeof(process_name));

            for (int i = 0; i < process_count; i++) {
                if (strcmp(process_name, process_names[i]) == 0) {
                    printf("Terminating process %s (PID: %d)\n", process_name, pid);
                    kill(pid, SIGKILL); // Terminate the process
                }
            }
        }
    }

    closedir(proc_dir);
}