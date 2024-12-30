#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ncurses.h>
#include <string.h>
#include <sys/file.h>
#include <time.h>
#include <stdarg.h>

#define TIMEOUT 3 // Timeout in seconds
#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_keyboard.txt"

/*======================================================*/
// API to update watchdog file
void update_watchdog_file();
/*======================================================*/
// API to clear content of log file
void clear_log_file(const char *filename);
/*======================================================*/
// API to appened content in log file
void append_to_log_file(const char *filename, const char *format, ...);
/*======================================================*/
int main()
{
    // clear log content
    clear_log_file(LOG_FILE_NAME);
    int fd_request_keyboard = open("./pipes/keyboard_Request", O_WRONLY);
    append_to_log_file(LOG_FILE_NAME, "the request pipe is opened successfully\n");
    int fd_response_keyboard = open("./pipes/keyboard_Respond", O_RDONLY);
    append_to_log_file(LOG_FILE_NAME, "the response pipe is opened successfully\n");

    if (fd_request_keyboard == -1)
    {
        perror("Failed to open pipe_keyboard");
        append_to_log_file(LOG_FILE_NAME, "files to open pipe keyboard\n");
        exit(1);
    }

    initscr();
    append_to_log_file(LOG_FILE_NAME, "init screen successed\n");
    cbreak();
    append_to_log_file(LOG_FILE_NAME, "cbreak successed\n");
    noecho();
    append_to_log_file(LOG_FILE_NAME, "noecho successed\n");
    keypad(stdscr, TRUE);
    append_to_log_file(LOG_FILE_NAME, "kaypad successed\n");
    start_color();
    append_to_log_file(LOG_FILE_NAME, "start_color successed\n");
    curs_set(0);
    append_to_log_file(LOG_FILE_NAME, "curs_set successed\n");
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_WHITE);
    append_to_log_file(LOG_FILE_NAME, "init pair successed\n");

    WINDOW *win1 = newwin(30, 40, 2, 2);
    WINDOW *win2 = newwin(30, 40, 2, 46);
    refresh();
    box(win1, 0, 0);

    wattron(win2, COLOR_PAIR(2));
    box(win2, 0, 0);

    append_to_log_file(LOG_FILE_NAME, "inilization is ready to accept shaped which we will define\n");
    // circle  shapes in RED(2)
    mvwprintw(win2, 9, 17, " @@@ ");
    mvwprintw(win2, 10, 16, "@@   @@");
    mvwprintw(win2, 11, 15, "@@  5  @@");
    mvwprintw(win2, 12, 16, "@@   @@");
    mvwprintw(win2, 13, 17, " @@@ ");

    wrefresh(win1);
    wrefresh(win2);
    wattroff(win2, COLOR_PAIR(2));

    // Arrow shapes as arrays of strings (one string per line)
    const char *up_arrow[] = {
        "    ^",
        "   @^^",
        "  ^^^^^",
        " ^^^^^@^",
        "    8 ",
        "    |"};
    const char *down_arrow[] = {
        "    |  ",
        "    2  ",
        " ^^^^^@^",
        "  ^@^^^ ",
        "   ^^^  ",
        "    ^   "};
    const char *left_arrow[] = {
        "    <",
        "  <§<",
        "<<<<< 4 ==",
        "  <<§",
        "    <"};
    const char *right_arrow[] = {
        "      >   ",
        "      §>> ",
        " == 6 >>>>>",
        "      >>§ ",
        "      >  "};

    const char *upr_arrow[] = {
        "*******",
        "  *****",
        " 9  ***",
        "      *"

    };
    const char *upl_arrow[] = {
        "*******",
        "***** ",
        "***  7",
        "*"

    };
    const char *downr_arrow[] = {
        "      *",
        " 3  ***",
        "  *****",
        "*******"};
    const char *downl_arrow[] = {
        "*",
        "***  1",
        "*****",
        "*******"};

    // Add the main arrows to the main window in GREEN(1)

    wattron(win2, COLOR_PAIR(1));

    // Display up arrow
    for (int i = 0; i < 6; i++)
    {
        mvwprintw(win2, 2 + i, 15, "%s", up_arrow[i]);
    }
    wrefresh(win2);

    // Display down arrow
    for (int i = 0; i < 6; i++)
    {
        mvwprintw(win2, 15 + i, 15, "%s", down_arrow[i]);
    }
    wrefresh(win2);

    // Display right arrow
    for (int i = 0; i < 5; i++)
    {
        mvwprintw(win2, 9 + i, 25, "%s", right_arrow[i]);
    }
    wrefresh(win2);

    // Display left arrow
    for (int i = 0; i < 5; i++)
    {
        mvwprintw(win2, 9 + i, 3, "%s", left_arrow[i]);
    }

    wattroff(win2, COLOR_PAIR(1));

    wrefresh(win2);

    // Add the secondary arrows to the main window in WHITE(3)

    wattron(win2, COLOR_PAIR(3));
    // Display UP R arrow
    for (int i = 0; i < 4; i++)
    {
        mvwprintw(win2, 4 + i, 26, "%s", upr_arrow[i]);
    }

    wrefresh(win2);

    // Display UP L arrow
    for (int i = 0; i < 4; i++)
    {
        mvwprintw(win2, 4 + i, 6, "%s", upl_arrow[i]);
    }

    wrefresh(win2);

    // Display DOWN R arrow
    for (int i = 0; i < 4; i++)
    {
        mvwprintw(win2, 15 + i, 26, "%s", downr_arrow[i]);
    }

    wrefresh(win2);

    // Display DOWN L arrow
    for (int i = 0; i < 4; i++)
    {
        mvwprintw(win2, 15 + i, 6, "%s", downl_arrow[i]);
    }

    mvwprintw(win2, 0, 2, " INPUT KEYS ");

    wattroff(win2, COLOR_PAIR(3));

    wattron(win2, COLOR_PAIR(4));
    mvwprintw(win2, 26, 14, " /!/!/!/!/ ");
    mvwprintw(win2, 25, 11, " press q to exit ");
    wattroff(win2, COLOR_PAIR(4));

    wrefresh(win2);
    refresh();
    // time_t last_logged_time = 0;

    int ch;
    timeout(100); // Set getch() to non-blocking with a 100 ms timeout

    time_t last_logged_time = time(NULL);
    while ((ch = getch()) != 'q')
    {
        time_t current_time = time(NULL);

        if (ch != ERR)
        { // If a key was pressed
            if (ch == '8')
            {
                write(fd_request_keyboard, "up", 3);
                append_to_log_file(LOG_FILE_NAME, "the up button is presses\n");
            }
            else if (ch == '2')
            {
                write(fd_request_keyboard, "down", 5);
                append_to_log_file(LOG_FILE_NAME, "the down button is presses\n");
            }
            else if (ch == '4')
            {
                write(fd_request_keyboard, "left", 5);
                append_to_log_file(LOG_FILE_NAME, "the left button is presses\n");
            }
            else if (ch == '6')
            {
                write(fd_request_keyboard, "right", 6);
                append_to_log_file(LOG_FILE_NAME, "the right button is presses\n");
            }
            else if (ch == '5')
            {
                write(fd_request_keyboard, "stop", 5);
                append_to_log_file(LOG_FILE_NAME, "the stop button is presses\n");
            }
            else if (ch == '7')
            {
                write(fd_request_keyboard, "up-left", 8);
                append_to_log_file(LOG_FILE_NAME, "the up-left button is presses\n");
            }
            else if (ch == '9')
            {
                write(fd_request_keyboard, "up-right", 9);
                append_to_log_file(LOG_FILE_NAME, "the up-right button is presses\n");
            }
            else if (ch == '1')
            {
                write(fd_request_keyboard, "down-left", 10);
                append_to_log_file(LOG_FILE_NAME, "the down-left button is presses\n");
            }
            else if (ch == '3')
            {
                write(fd_request_keyboard, "down-right", 11);
                append_to_log_file(LOG_FILE_NAME, "the down-right button is presses\n");
            }
            else if (ch == 't')
            {
                write(fd_request_keyboard, "target", 7);
                append_to_log_file(LOG_FILE_NAME, "the t button is presses\n");
            }
            else if (ch == 'o')
            {
                write(fd_request_keyboard, "obsticale", 10);
                append_to_log_file(LOG_FILE_NAME, "the o button is presses\n");
            }
            else if (ch == 'r')
            {
                write(fd_request_keyboard, "reset", 6);
            }
        }

        if (current_time - last_logged_time >= TIMEOUT)
        {
            update_watchdog_file();
            last_logged_time = current_time;
        }
    }
    endwin();
    return 0;
    close(fd_request_keyboard);
    close(fd_response_keyboard);
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