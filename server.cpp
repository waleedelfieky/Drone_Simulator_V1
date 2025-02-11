//==========================================================
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
//==========================================================
// DDS headers
#include "Generated/TargetsPubSubTypes.hpp"
#include "Generated/ObstaclesPubSubTypes.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

//==========================================================
// define macros
#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_server.txt"
#define READWRITEPERMISSION 0666
#define DEFAULT_FDVALUE 0
#define pair 2
#define NUMBER_OF_CLIENTS 3 // Number of clients
#define keyboard_pipe_index 0
#define visualizer_pipe_index 1
#define drone_pipe_index 2
#define MAX_TARGETS 10
#define MAX_OBSTACLES 20
#define TIMEOUT 8 // Timeout in seconds
//==========================================================

// structures

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
} Target;

typedef struct
{
    int x, y;
    int size;
    int active;
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

struct PIPES_T
{
    const char *request_path;
    const char *response_path;
    int permission;
    int fd_request;
    int fd_response;
};

//=====================================================================
// APIs
/*======================================================*/
// function to update watchdog file
void update_watchdog_file();
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
// Helper funcyiond
int max_request_fd(struct PIPES_T **pipes_paths, int number_of_clients);
void fill_read_fds(struct PIPES_T **pipes_paths, int number_of_clients, fd_set *readfds_l);
/*======================================================*/
// associate every key pressed with certin tasks
void keyboard_pipe_Work(char *keyboard_input);
/*======================================================*/
// initlize the system
void init(void);
/*======================================================*/
// send signal to target
void send_target_signal();
/*======================================================*/
// send signal to obsticale
void send_obsticale_signal();
/*======================================================*/
// API to clear content of log file
void clear_log_file(const char *filename);
/*======================================================*/
// API to appened content in log file
void append_to_log_file(const char *filename, const char *format, ...);
/*======================================================*/

/*======================================================*/
// Create global varibales
SharedState state = {0};
// Set the first target to be collected as 1
SharedState reset_state = {0};

Target generated_targets[MAX_TARGETS];
Obstacle generated_obsticales[MAX_OBSTACLES];

const int NUMBER_OF_PIPES = NUMBER_OF_CLIENTS * 2;
int closed_pipe_flags[NUMBER_OF_CLIENTS] = {0};

// global varibales contains the fifos info
struct PIPES_T keyboard_pipe = {"./pipes/keyboard_Request", "./pipes/keyboard_Respond", READWRITEPERMISSION, DEFAULT_FDVALUE, DEFAULT_FDVALUE};
struct PIPES_T visualizer_pipe = {"./pipes/visualizer_Request", "./pipes/visualizer_Respond", READWRITEPERMISSION, DEFAULT_FDVALUE, DEFAULT_FDVALUE};
struct PIPES_T drone_pipe = {"./pipes/drone_Request", "./pipes/drone_Respond", READWRITEPERMISSION, DEFAULT_FDVALUE, DEFAULT_FDVALUE};
struct PIPES_T *pipes_paths[NUMBER_OF_CLIENTS] = {&keyboard_pipe, &visualizer_pipe, &drone_pipe};
/*======================================================*/
// DDS classes
// unpack the namespaces here to use it later
using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

// Subscriber Targets class
class TargetsSubscriber
{
private:
    DomainParticipant *participant_;

    Subscriber *subscriber_;

    DataReader *reader_;

    Topic *topic_;

    TypeSupport type_;

    class SubListener : public DataReaderListener
    {
    public:
        SubListener()
            : samples_(0)
        {
        }

        ~SubListener() override
        {
        }

        void on_subscription_matched(
            DataReader *reader,
            const SubscriptionMatchedStatus &info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "Subscriber matched." << std::endl;
                eprosima::fastdds::rtps::LocatorList locators;
                reader->get_listening_locators(locators);
                for (const eprosima::fastdds::rtps::Locator &locator : locators)
                {
                    print_transport_protocol(locator);
                }
            }
            else if (info.current_count_change == -1)
            {
                std::cout << "Subscriber unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                          << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
            }
        }

        void on_data_available(DataReader *reader) override
        {
            SampleInfo info;
            if (reader->take_next_sample(&targets_, &info) == eprosima::fastdds::dds::RETCODE_OK)
            {
                if (info.valid_data)
                {
                    samples_++;
                    std::cout << "Received " << targets_.targets_number() << " targets:" << std::endl;

                    // Process each target
                    for (size_t i = 0; i < targets_.targets_x().size(); ++i)
                    {
                        generated_targets[i].x = targets_.targets_x()[i];
                        generated_targets[i].y = targets_.targets_y()[i];
                        generated_targets[i].value = i;
                        generated_targets[i].active = 1;
                        std::cout << "Target " << i + 1 << ": ("
                                  << generated_targets[i].x << ", "
                                  << generated_targets[i].y << ")" << std::endl;

                        // assign the reccieved data to the global array of struct that would be send later to the other processes
                        // this assignation happens under condition that the flag is matched
                        memcpy(state.targets, generated_targets, sizeof(generated_targets));
                    }
                }
            }
        }

    public:
        Targets targets_;

        std::atomic_int samples_;

    private:
        void print_transport_protocol(const eprosima::fastdds::rtps::Locator &locator)
        {
            switch (locator.kind)
            {
            case LOCATOR_KIND_UDPv4:
                std::cout << "Using UDPv4" << std::endl;
                break;
            case LOCATOR_KIND_UDPv6:
                std::cout << "Using UDPv6" << std::endl;
                break;
            case LOCATOR_KIND_SHM:
                std::cout << "Using Shared Memory" << std::endl;
                break;
            default:
                std::cout << "Unknown Transport" << std::endl;
                break;
            }
        }

    } listener_;

public:
    TargetsSubscriber()
        : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr), type_(new TargetsPubSubType())
    {
    }

    virtual ~TargetsSubscriber()
    {
        if (reader_ != nullptr)
        {
            subscriber_->delete_datareader(reader_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    //! Initialize the subscriber
    bool init()
    {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");

        participant_ = DomainParticipantFactory::get_instance()->create_participant(1, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        // Register the Type
        type_.register_type(participant_);

        // Create the subscriptions Topic
        topic_ = participant_->create_topic("TargetsTopic", type_.get_type_name(), TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            return false;
        }

        // Create the Subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (subscriber_ == nullptr)
        {
            return false;
        }

        // Create the DataReader
        reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

        if (reader_ == nullptr)
        {
            return false;
        }

        return true;
    }
};

// Obsticale Subscriber class

class ObstaclesSubscriber
{
private:
    DomainParticipant *participant_;
    Subscriber *subscriber_;
    DataReader *reader_;
    Topic *topic_;
    TypeSupport type_;

    class SubListener : public DataReaderListener
    {
    public:
        SubListener()
            : samples_(0)
        {
        }

        ~SubListener() override
        {
        }

        void on_subscription_matched(
            DataReader *reader,
            const SubscriptionMatchedStatus &info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "Subscriber matched." << std::endl;
                eprosima::fastdds::rtps::LocatorList locators;
                reader->get_listening_locators(locators);
                for (const eprosima::fastdds::rtps::Locator &locator : locators)
                {
                    print_transport_protocol(locator);
                }
            }
            else if (info.current_count_change == -1)
            {
                std::cout << "Subscriber unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                          << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
            }
        }

        void on_data_available(DataReader *reader) override
        {
            SampleInfo info;
            if (reader->take_next_sample(&obstacles_, &info) == eprosima::fastdds::dds::RETCODE_OK)
            {
                if (info.valid_data)
                {
                    samples_++;
                    std::cout << "Received " << obstacles_.obstacles_number() << " obstacles:" << std::endl;

                    // Process each obstacle
                    for (size_t i = 0; i < obstacles_.obstacles_x().size(); ++i)
                    {
                        generated_obsticales[i].x = obstacles_.obstacles_x()[i];
                        generated_obsticales[i].y = obstacles_.obstacles_y()[i];
                        generated_obsticales[i].size = 1;
                        generated_obsticales[i].active = 1;
                        std::cout << "Obstacle " << i + 1 << ": ("
                                  << generated_obsticales[i].x << ", "
                                  << generated_obsticales[i].y << ")" << std::endl;
                        // Assign new target values to the global array of struct when flag is matched
                        memcpy(state.obstacles, generated_obsticales, sizeof(generated_obsticales));
                    }
                }
            }
        }

    public:
        Obstacles obstacles_;
        std::atomic_int samples_;

    private:
        void print_transport_protocol(const eprosima::fastdds::rtps::Locator &locator)
        {
            switch (locator.kind)
            {
            case LOCATOR_KIND_UDPv4:
                std::cout << "Using UDPv4" << std::endl;
                break;
            case LOCATOR_KIND_UDPv6:
                std::cout << "Using UDPv6" << std::endl;
                break;
            case LOCATOR_KIND_SHM:
                std::cout << "Using Shared Memory" << std::endl;
                break;
            default:
                std::cout << "Unknown Transport" << std::endl;
                break;
            }
        }

    } listener_;

public:
    ObstaclesSubscriber()
        : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr), type_(new ObstaclesPubSubType())
    {
    }

    virtual ~ObstaclesSubscriber()
    {
        if (reader_ != nullptr)
        {
            subscriber_->delete_datareader(reader_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    //! Initialize the subscriber
    bool init()
    {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");

        participant_ = DomainParticipantFactory::get_instance()->create_participant(1, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        // Register the Type
        type_.register_type(participant_);

        // Create the subscriptions Topic
        topic_ = participant_->create_topic("ObstaclesTopic", type_.get_type_name(), TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            return false;
        }

        // Create the Subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (subscriber_ == nullptr)
        {
            return false;
        }

        // Create the DataReader
        reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

        if (reader_ == nullptr)
        {
            return false;
        }

        return true;
    }
};

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
    // create instance of Subscriber target and obstacles
    std::cout << "Starting Obstacle subscriber." << std::endl;
    ObstaclesSubscriber *my_obstacles_sub = new ObstaclesSubscriber();
    my_obstacles_sub->init();
    std::cout << "Starting Target subscriber." << std::endl;
    TargetsSubscriber *my_targets_sub = new TargetsSubscriber();
    my_targets_sub->init();

    clear_log_file(LOG_FILE_NAME);
    // assign default key as "stop"
    // strcpy(state.key_pressed,"stop");
    state.drone.x = 10;
    state.drone.y = 10;
    state.drone.fx = 0;
    state.drone.fy = 0;
    state.drone.vx = 0;
    state.drone.vy = 0;
    state.next_target = 1;
    state.attemp = 1;
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

    // state.next_target = 1; // Set the first target to be collected as 1

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
                    // printf("===========================================================\n");
                    //  printf("Drone -> Position: (%.2f, %.2f), Velocity: (%.2f, %.2f), Force: (%.2f, %.2f)\n"
                    //         "Score: %d\n"
                    //         "Message: %s\n",
                    //         state.drone.x, state.drone.y,
                    //         state.drone.vx, state.drone.vy,
                    //         state.drone.fx, state.drone.fy,
                    //         state.score,
                    //         state.message);
                    //  printf("===========================================================\n");
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
        // set score to zero
        state.score = 0;
        state.next_target = 1;
        state.attemp = state.attemp + 1;
        // send signals to generat new targets and obsticales
        send_target_signal();
        append_to_log_file(LOG_FILE_NAME, "targets generated successfuly\n");
        send_obsticale_signal();
        append_to_log_file(LOG_FILE_NAME, "obsticales generated successfuly\n");
    }
    else if (strcmp(keyboard_input, "request_data") == 0)
    {
        // printf("struct is sent successfully to keyboard\n");
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
    // implement our logic here
    //  upadte in global varibale
    // memcpy(state.targets, target_generator_reader, sizeof(target_generator_reader));
}

void send_obsticale_signal()
{
    // implement our logic here
    // memcpy(state.obstacles, obsticale_generator_reader, sizeof(obsticale_generator_reader));
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