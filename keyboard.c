#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ncurses.h>
#include <string.h>

#include <sys/file.h>
#include <string.h>
#include <time.h>

#define TIMEOUT 4 // Timeout in seconds
#define LOG_FILE "watchdog_log.txt"

void update_watchdog_file();

int main()
{
    int fd_request_keyboard = open("./pipes/keyboard_Request", O_WRONLY);
    int fd_response_keyboard = open("./pipes/keyboard_Respond", O_RDONLY);

    if (fd_request_keyboard == -1)
    {
        perror("Failed to open pipe_keyboard");
        exit(1);
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    curs_set(0);
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_WHITE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_WHITE);

    WINDOW *win1 = newwin(30, 40, 2, 2);
    WINDOW *win2 = newwin(30, 40, 2, 46);
    refresh();
    box(win1, 0, 0);

    wattron(win2, COLOR_PAIR(2));
    box(win2, 0, 0);

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
    time_t last_logged_time = 0;

    int ch;
    while ((ch = getch()) != 'q')
    {
        if (ch == '8')
        {
            write(fd_request_keyboard, "up", 3);
        }
        else if (ch == '2')
        {
            write(fd_request_keyboard, "down", 5);
        }
        else if (ch == '4')
        {
            write(fd_request_keyboard, "left", 5);
        }
        else if (ch == '6')
        {
            write(fd_request_keyboard, "right", 6);
        }
        else if (ch == '5')
        {
            write(fd_request_keyboard, "stop", 5);
        }
        else if (ch == '7')
        {
            write(fd_request_keyboard, "up-left", 8);
        }
        else if (ch == '9')
        {
            write(fd_request_keyboard, "up-right", 9);
        }
        else if (ch == '1')
        {
            write(fd_request_keyboard, "down-left", 10);
        }
        else if (ch == '3')
        {
            write(fd_request_keyboard, "down-right", 11);
        }
        else if (ch == 't')
        {
            write(fd_request_keyboard, "target", 7);
        }
        else if (ch == 'o')
        {
            write(fd_request_keyboard, "obsticale", 10);
        }
        time_t current_time = time(NULL);
        if (current_time - last_logged_time >= TIMEOUT)
        { // Check if 3 seconds have passed
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