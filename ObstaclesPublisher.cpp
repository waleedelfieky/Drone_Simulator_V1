#include "Generated/ObstaclesPubSubTypes.hpp"

#include <chrono>
#include <thread>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()
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

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#define TIMEOUT 4 // Timeout in seconds
#define LOG_FILE "watchdog_log.txt"
#define LOG_FILE_NAME "log_obsticale.txt"
#define MAX_OBSTACLES 20 // Maximum number of obstacles to generate

// Function Declarations
void update_watchdog_file();
void clear_log_file(const char *filename);
void append_to_log_file(const char *filename, const char *format, ...);

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

class ObstaclesPublisher
{
private:
    Obstacles obstacles_;

    DomainParticipant *participant_;
    Publisher *publisher_;
    Topic *topic_;
    DataWriter *writer_;
    TypeSupport type_;

    class PubListener : public DataWriterListener
    {
    public:
        PubListener()
            : matched_(0)
        {
        }

        ~PubListener() override
        {
        }

        void on_publication_matched(
            DataWriter *writer,
            const PublicationMatchedStatus &info) override
        {
            if (info.current_count_change == 1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher matched." << std::endl;
                eprosima::fastdds::rtps::LocatorList locators;
                writer->get_sending_locators(locators);
                for (const eprosima::fastdds::rtps::Locator &locator : locators)
                {
                    print_transport_protocol(locator);
                }
            }
            else if (info.current_count_change == -1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                          << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
            }
        }

        std::atomic_int matched_;

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
    ObstaclesPublisher()
        : participant_(nullptr), publisher_(nullptr), topic_(nullptr), writer_(nullptr), type_(new ObstaclesPubSubType())
    {
    }

    virtual ~ObstaclesPublisher()
    {
        if (writer_ != nullptr)
        {
            publisher_->delete_datawriter(writer_);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    //! Initialize the publisher
    bool init()
    {
        obstacles_.obstacles_number(0);

        DomainParticipantQos participantQos;
        participantQos.name("Participant_publisher");

        participant_ = DomainParticipantFactory::get_instance()->create_participant(1, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        // Register the Type
        type_.register_type(participant_);

        // Create the publications Topic
        topic_ = participant_->create_topic("ObstaclesTopic", type_.get_type_name(), TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            return false;
        }

        // Create the Publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

        if (publisher_ == nullptr)
        {
            return false;
        }

        // Create the DataWriter
        writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &listener_);

        if (writer_ == nullptr)
        {
            return false;
        }
        return true;
    }

    //! Send a publication
    bool publish()
    {
        // Clear previous obstacles
        obstacles_.obstacles_x().clear();
        obstacles_.obstacles_y().clear();

        // Seed for random numbers
        srand(time(NULL));

        if (listener_.matched_ > 0)
        {
            for (int i = 0; i < MAX_OBSTACLES; ++i)
            {
                // Generate odd numbers for x (1–79) and y (1–19)
                int x = (rand() % 45) * 2 + 1; // Random odd x position (1–79)
                int y = (rand() % 20) * 2 + 1; // Random odd y position (1–19)

                // Add the coordinates to the Obstacles message
                obstacles_.obstacles_x().push_back(x);
                obstacles_.obstacles_y().push_back(y);

                // Print the generated obstacle (for debugging)
                std::cout << "Generated Obstacle " << i + 1 << ": (" << x << ", " << y << ")" << std::endl;
            }

            // Set the number of obstacles
            obstacles_.obstacles_number(obstacles_.obstacles_x().size());

            // Publish the message
            writer_->write(&obstacles_);
            return true;
        }
        else
        {
            std::cout << "No subscribers found." << std::endl;
        }
        // return false;
    }
};

int main()
{
    clear_log_file(LOG_FILE_NAME);

    std::cout << "Starting publisher." << std::endl;
    std::cout<<"file is called\n";
    append_to_log_file(LOG_FILE_NAME, "Starting publisher.\n");

    ObstaclesPublisher *mypub = new ObstaclesPublisher();

    mypub->init();
    time_t last_logged_time = 0;

    while (true)
    {
        time_t current_time = time(NULL);
        if (current_time - last_logged_time >= TIMEOUT)
        {
            append_to_log_file(LOG_FILE_NAME, "Watchdog has been updated with new code.\n");
            update_watchdog_file();
            last_logged_time = current_time;
        }

        mypub->publish();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }

    delete mypub;
    return 0;
}

// Initialize Functions

// Watchdog File Update Function
void update_watchdog_file()
{
    FILE *file = fopen(LOG_FILE, "r+");
    if (!file)
    {
        file = fopen(LOG_FILE, "w");
        if (!file)
        {
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
            fprintf(temp_file, "%d,%ld\n", pid, current_time);
            found = 1;
        }
        else
        {
            fputs(line, temp_file);
        }
    }

    if (!found)
    {
        fprintf(temp_file, "%d,%ld\n", pid, current_time);
    }

    rewind(temp_file);
    freopen(LOG_FILE, "w", file);
    while (fgets(line, sizeof(line), temp_file))
    {
        fputs(line, file);
    }

    fclose(temp_file);
    flock(fd, LOCK_UN);
    fclose(file);
}

// Clear Log File Function
void clear_log_file(const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Error opening log file to clear");
        exit(EXIT_FAILURE);
    }
    fclose(file);
}

// Append to Log File Function
void append_to_log_file(const char *filename, const char *format, ...)
{
    int fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0)
    {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    if (flock(fd, LOCK_EX) < 0)
    {
        perror("Error locking file");
        close(fd);
        exit(EXIT_FAILURE);
    }

    FILE *file = fdopen(fd, "a");
    if (file == NULL)
    {
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
