# Drone_Simulator_V1

## Table of Contents:
- [overview-of-system](#overview-of-system)
- [Watchdog_process](#Watchdog_process)
- [Server_Node](#Server_Node)
- [Target_Generator_Node](#Target_Generator_Node)
- [Obstcale_Generator_Node](#Obstcale_Generator_Node)
- [Dynamic_Node](#Dynamic_Node)
- [Keyboard_Node](#Keyboard_Node)
- [Visualizer_Node](#Visualizer_Node)
- [video](#video)
- [Contact Information](#contact-information)

## overview-of-system:
### Overall system Overview
![image](https://github.com/user-attachments/assets/af120573-d717-4359-9e5f-72b8471cfb19)

The system comprises five processes, each connected to a server process through a request-response mechanism. Each process can send requests to the server, which in turn responds accordingly. In this setup, requests and responses consist of data that is processed either on the node side or the server side. Communication between the server and the nodes is facilitated through named pipes, ensuring seamless data exchange. 

The system employs the select function to guarantee non-blocking behavior, allowing a smooth flow of operations. Additionally, the flow may be interrupted by signal commands issued by the user through a keyboard pipe. These signals are processed by the server, which determines an appropriate time to execute the interrupt routine without disrupting the overall system flow. This ensures the system remains responsive while maintaining consistent operation. 

The parameters file is used with dynamic node, also each process is logging its progress to a log file so user can track what’s going on at any time or in case of any error, a make file is used to build the code and launcher file is used to first build system using the make file then do some initialization and then run the system. 
### Each process responsibilities: 
**Dynamic Node**: Responsible for position, forces, velocities, accelerations of the system 
**Keyboard Node**: Responsible for user friendly GUI interaction 
**Visualizer Node**: Responsible for Visualization whole environment with drone motion 
**Target Generator**: Responsible For Generating targets values and positions 
**Obstacles Generator**: Responsible for Generating obstacles positions  
**Watchdog**: watch if any process stacked or crashed then close the whole system 

## Watchdog_process:
**Diagram**:
![image](https://github.com/user-attachments/assets/7023fe5a-b303-4b2a-bac4-787ef3cdaf5b)

Each of the five processes, along with the server process, periodically writes its last execution timestamp to a shared file. This shared file serves as a centralized log for monitoring the activity of all processes in the system. The watchdog process continuously monitors this shared file to ensure that each process is actively writing its timestamp within a predefined time limit. Each process when writing to the file used file Locking to prevent race conditions 

If the watchdog process detects that any of the six processes has exceeded the allowed time limit without updating its timestamp in the shared file, it takes corrective action. Specifically, the watchdog terminates the entire system by sending kill signals to all six running processes. Once this is done, it informs the user about the termination event and the reason behind it, ensuring transparency in system operations. 

During its execution, the watchdog process also provides real-time feedback to the user by displaying the last recorded execution times of all processes on the terminal. This allows the user to monitor the flow and activity of the processes as they execute, offering a clear and detailed view of the system's status. The following diagram illustrates this monitoring mechanism and the flow of information. 
![image](https://github.com/user-attachments/assets/1abd73f6-7f3f-463e-b910-2078022cd675)

## Server_Node:

The server node acts as the central hub within the system, responsible for managing and coordinating all processes. It communicates with the five individual processes through named pipes, which serve as inter-process communication channels. The server node's primary role is to receive data from one or more of these processes, process or analyze the information as required, and then distribute the results or relevant data back to the other processes. This centralized management ensures that the system operates efficiently and maintains synchronization between all interconnected components. Additionally, the server node's design allows for streamlined communication, enabling seamless data transfer and reducing potential conflicts or delays in the system's operation. 
![image](https://github.com/user-attachments/assets/3fe2d630-a278-44bc-af1c-3f375af7671b)
Init will initialize the system as shown in the figure and create fifo will create the pipes if they do not exist and open will open those pipes. For select monitor we would discuss it in detail in the following:

### Select Monitor - Main Functionalities

#### Features

#### Monitor Multiple Pipes
- Utilizes the `select` system call to monitor multiple client pipes for incoming requests.

#### Handle Keyboard Requests
- Reads commands from the keyboard pipe.
- Responds with the current state and processes user inputs.

#### Handle Visualizer Requests
- Reads data from the visualizer pipe.
- Updates the state and sends the updated data back.

#### Handle Drone Requests
- Reads the drone's position and updates its state.
- Calculates distances to targets for scoring and updates target statuses.

#### Handle Obstacle Updates
- Reads obstacle data from the respective pipe.
- Updates the shared state with the new obstacle information.

#### Log and Track Pipe Status
- Detects and logs when pipes are closed by writers.
- Skips processing for closed pipes.

#### Periodic Watchdog Updates
- Updates a watchdog file every 3 seconds to indicate activity.

#### Error Handling
- Manages errors in `select` and handles timeouts gracefully.

**Server handles signals too After getting the command from keyboard node as following diagram**: 
![image](https://github.com/user-attachments/assets/137aad1e-b9b3-4c94-9d7e-d850b0ffdd88)

## Target_Generator_Node
This node simply notifies the server first by its PID so the server can use it later and then at any time the server wants targets it sends a signal to the target generation node, and it responds in return by sending 10 random targets with their positions. Also, it updates it’s log data and watchdog shared file memory. 

## Obstcale_Generator_Node
This node simply notifies the server first by its PID so the server can use it later and then at any time the server wants obstacles it sends a signal to the obstacle generation node, and it responds back by sending random obstacles with their positions. Also, it updates it’s log data and watchdog shared file memory. This node periodically sent obstacles to the server so obstacles in the environment change periodically with time or when obstacle generation key is pressed. 

## Dynamic_Node
**Diagram**
![image](https://github.com/user-attachments/assets/19548e00-3a30-44f4-a602-3b88d281ea34)

#### Overview

The provided schematic represents the main workflow of the dynamic node, which is designed to simulate and control the dynamics of a drone. It highlights the initialization process, communication setup, and the main control loop.

### Workflow

### Start
The node begins by initializing parameters and setting up the environment for the simulation.

### Initialize Parameters
- The node reads simulation parameters (e.g., mass, maximum force, geofence limits) from a configuration file (`parameters.txt`).
- The parameters are logged for reference.

### Open Pipes for Drone Communication
- Pipes are used for inter-process communication with the server (`PIPE_REQUEST_DRONE` and `PIPE_RESPONSE_DRONE`).
- If the pipes fail to open, an error is logged, and the program exits.

### Main Loop
1. **Send Drone State to Server**  
   - The drone's current state (position, velocity, forces) is sent to the server via the request pipe.

2. **Receive State from Server**  
   - The node reads updates from the server, including target and obstacle data.

3. **Update Drone Dynamics**  
   - The drone's dynamics are recalculated based on user inputs, obstacles, and geofence constraints using `update_Drone_dynamics` (described below).

4. **Watchdog Timeout Check**  
   - Periodically, the node updates the watchdog log file to indicate active operation.

5. **Continue Loop**  
   - The node loops back to sending the updated drone state to the server.

### Update Drone Dynamics: 
**Diagram**
![image](https://github.com/user-attachments/assets/68a54f06-4824-42ec-8d3e-cf878508e4ba)

### Process Workflow

1. **Initialization**
   - The process begins by initializing all forces related to the drone. These include:
     - `F_rep_x`, `F_rep_y`: Repulsive forces from obstacles and geofence boundaries.

2. **Key Press Detection**
   - The process checks if a key press is detected (`key_pressed` field in the `SharedState` structure):
     1. If a key press is detected:
        - The corresponding force is applied based on the key input (e.g., "up", "down", "left", "right"), as shown below.
     2. If no key press is detected:
        - No input forces are applied, and the process proceeds to check obstacles.

3. **Obstacle Interaction**
   - The process checks for obstacles within the drone’s perception radius (`rho`):
     1. **If obstacles are detected:**
        - Repulsive forces (`F_rep_x`, `F_rep_y`) are calculated based on the obstacle’s position and distance from the drone:
          \[
          F_{rep_x} = \left( \frac{1}{d_{obs}^2} \right) \cdot \frac{x_{obs} - x}{distance}
          \]
          \[
          F_{rep_y} = \left( \frac{1}{d_{obs}^2} \right) \cdot \frac{y_{obs} - y}{distance}
          \]
     2. **If no obstacles are detected:**
        - The process skips obstacle repulsion and moves to geofence boundary checks.

4. **Geofence Boundary Check**
   - The process ensures the drone stays within the geofence boundaries (`x_max`, `y_max`):
     1. If the drone is near a boundary, repulsive forces are added to push it back within the boundaries.

5. **Combine Forces**
   - All forces are combined, including:
     - Repulsive forces (`F_rep`).
     - Forces applied due to key inputs.

6. **Update Velocity and Position**
   - **Velocity Update:**
     - The drone’s velocity is updated using the calculated acceleration:
       \[
       a = \frac{F_{total}}{M}
       \]
   - **Position Update:**
     - The drone’s position is updated based on the new velocity:
       \[
       x = x + v \cdot T
       \]
       \[
       y = y + v \cdot T
       \]

7. **Apply Boundary Constraints**
   - The program ensures the drone remains within the geofence limits. If violations are detected, the position is adjusted accordingly.


8. **Update Shared State**
   - The updated state (position, velocity, and forces) are written back to the `SharedState` structure for use in the next loop iteration.


## Keyboard_Node

![image](https://github.com/user-attachments/assets/4be44950-ae7f-4edd-ac83-6807df2f88d4)
# Keyboard-Based GUI and Server Interaction System

## Overview
This program facilitates interaction between a **keyboard interface**, **GUI windows**, and a **server** to control actions, display metrics, and manage signals in real-time.

## Features
1. **Dual GUI Windows:**
   - **Metrics Display Window:** Shows real-time updates for:
     - Score
     - Position
     - Velocity
     - Forces
     - Attempt number
   - **Input Guidance Window:** Provides user-friendly guidance on which keys correspond to specific actions.

2. **Input Commands:**
   - **Movement Keys (1-9):**
     - `1`: Down Left
     - `2`: Down
     - `3`: Down Right
     - `4`: Left
     - `5`: Stop
     - `6`: Right
     - `7`: Up Left
     - `8`: Up
     - `9`: Up Right
   - **Special Commands:**
     - `t`: Target Generation Signal
     - `o`: Obstacle Generation Signal
     - `r`: Reset Signal
   - Inputs are processed and sent to the **server** to execute the corresponding actions.

3. **Validation Mechanism:**
   - Invalid inputs are ignored, ensuring the system flow is not disrupted.
   - All invalid entries are logged for monitoring and debugging.

4. **Server Interaction:**
   - All valid commands are transmitted to the server using **named pipes**.
   - The server processes the input and updates the GUI with the latest state.

## How It Works
1. **Initialization:**
   - The program initializes two GUI windows for displaying metrics and providing input guidance.
   
2. **User Input Handling:**
   - The program listens for keyboard inputs.
   - Valid inputs (`1-9`, `t`, `o`, `r`) are processed to their respective commands.
   - Invalid inputs are ignored and logged.

3. **Server Communication:**
   - Valid commands are sent to the server through **named pipes**.
   - The server performs the appropriate actions based on the input.

4. **Feedback Loop:**
   - The server continuously sends updates to the GUI windows to ensure real-time feedback for the user.

5. **Watch Dog Log update:**
   - Update watchdog log file by last execution time.

## Key Advantages
- **User-Friendly Interface:** Clearly separates metric display and input guidance for better usability.
- **Real-Time Updates:** Ensures the system state is always reflected accurately in the GUI.
- **Robust Validation:** Prevents disruptions due to invalid inputs and logs them for analysis.
- **Seamless Server Interaction:** Efficiently communicates user inputs to the server for processing.

## Input Reference
| Key | Action                          |
|-----|---------------------------------|
| 1   | Move Down Left                  |
| 2   | Move Down                       |
| 3   | Move Down Right                 |
| 4   | Move Left                       |
| 5   | Stop                            |
| 6   | Move Right                      |
| 7   | Move Up Left                    |
| 8   | Move Up                         |
| 9   | Move Up Right                   |
| r   | Reset Signal                    |
| t   | Target Generation Signal        |
| o   | Obstacle Generation Signal      |

## Logging and Error Handling
- Invalid inputs are logged without disrupting the flow.
- The system ensures fault tolerance by ignoring unrecognized inputs and maintaining steady communication with the server.

---

This program provides a robust and user-friendly interface for managing real-time system interactions between the GUI and the server.


## Visualizer_Node
**Diagram**
![image](https://github.com/user-attachments/assets/66dbe939-8983-4212-b1f1-0641beac7acb)
The program begins by initializing required resources, including clearing the log file and preparing for inter-process communication.  
### Visualization System

1. **Init:**

   The program begins by initializing required resources, including clearing the log file and preparing for inter-process communication.

2. **Clear Log File**

   The log file used for logging visualization activities (`log_visualizer.txt`) is cleared at the start of the program to ensure clean logs for the new session.

---

3. **Open Pipes for Visualization Communication**

   - **Request Pipe (`PIPE_REQUEST_VISUALIZATION`)**: Used to send requests to the server.
   - **Response Pipe (`PIPE_RESPONSE_VISUALIZATION`)**: Used to receive the updated shared state from the server.

   If either pipe fails to open, the program logs the error and exits.

4. **Initialize `ncurses`**

   If the pipes are successfully opened, the `ncurses` library is initialized to create a terminal-based graphical interface:

   - `initscr()`: Initializes the `ncurses` window.
   - **Color Initialization**:
     - **Blue** (`COLOR_PAIR(1)`) for the drone.
     - **Green** (`COLOR_PAIR(2)`) for targets.
     - **Red** (`COLOR_PAIR(3)`) for obstacles.

5. **Create Visualization Window**

   A new `ncurses` window is created based on the terminal’s size. This window is used to display the simulation’s graphical output.

---

### Main Loop

6. **Send Request to Server**

   The program sends a request message through the request pipe to prompt the server for the latest shared state.

7. **Receive Shared State from Server**

   The program reads the shared state structure from the response pipe. This state contains:
   - Drone position, velocity, and forces.
   - Active targets and their positions.
   - Obstacles and their positions and sizes.

8. **Display Visualization**

   The display function uses the received shared state to render:
   - The drone’s position (`+` symbol in blue).
   - The active targets (value symbol in green).
   - The obstacles (`O` symbol in red) as a block of characters based on their size.

   The `ncurses` window is updated with the current visualization.

9. **Watchdog Timeout**

   The program periodically checks if the **watchdog timeout** has been reached (default: 8 seconds). If so:
   - **Update Watchdog File**: The program updates the watchdog file to indicate that the process is still active.
   - If the timeout has not been reached, the program continues looping.
![image](https://github.com/user-attachments/assets/420d8d2f-9ecb-407e-a1b7-690644e86591)


## video for the final output:

## Contact Information:

If you have any questions, suggestions, or feedback regarding these this project, please feel free to contact me at waleedelfieky@gmail.com

