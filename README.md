# Drone_Simulator_V1

## Table of Contents:
- [System_in_action](#System_in_action)
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
## System_in_action

![image](https://github.com/user-attachments/assets/c97985e6-5255-46af-aa15-7f07394ce1f8)

This drone simulator allows user-controlled movement via a keyboard interface, with real-time updates on force, velocity, position, score, and attempt displayed on the screen. The drone avoids dynamically generated obstacles and collects targets to increase the score, following a modular architecture based on a request-response model. Communication between components, including visualization, keyboard input, obstacle generation, target generation and Dynamic, is handled via named pipes, with a central server managing these interactions and sending signals to coordinate updates. The system is designed for real-time feedback and efficient inter-process communication.

this is a drone simulator system
## overview-of-system:
### Overall system Overview
![image](https://github.com/user-attachments/assets/af120573-d717-4359-9e5f-72b8471cfb19)
<div style="text-indent: 40px;">
The system comprises five processes, each connected to a server process through a request-response mechanism. Each process can send requests to the server, which in turn responds accordingly. In this setup, requests and responses consist of data that is processed either on the node side or the server side. Communication between the server and the nodes is facilitated through named pipes, ensuring seamless data exchange. 

The system employs the select function to guarantee non-blocking behavior, allowing a smooth flow of operations. Additionally, the flow may be interrupted by signal commands issued by the user through a keyboard pipe. These signals are processed by the server, which determines an appropriate time to execute the interrupt routine without disrupting the overall system flow. This ensures the system remains responsive while maintaining consistent operation. 

The parameters file is used with dynamic node, also each process is logging its progress to a log file so user can track what’s going on at any time or in case of any error, a make file is used to build the code and launcher file is used to first build system using the make file then do some initialization and then run the system. 
</div>

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

<div style="text-indent: 40px;">
Each of the five processes, along with the server process, periodically writes its last execution timestamp to a shared file. This shared file serves as a centralized log for monitoring the activity of all processes in the system. The watchdog process continuously monitors this shared file to ensure that each process is actively writing its timestamp within a predefined time limit. Each process when writing to the file used file Locking to prevent race conditions 

If the watchdog process detects that any of the six processes has exceeded the allowed time limit without updating its timestamp in the shared file, it takes corrective action. Specifically, the watchdog terminates the entire system by sending kill signals to all six running processes. Once this is done, it informs the user about the termination event and the reason behind it, ensuring transparency in system operations. 

During its execution, the watchdog process also provides real-time feedback to the user by displaying the last recorded execution times of all processes on the terminal. This allows the user to monitor the flow and activity of the processes as they execute, offering a clear and detailed view of the system's status. The following diagram illustrates this monitoring mechanism and the flow of information. 
</div>

![image](https://github.com/user-attachments/assets/1abd73f6-7f3f-463e-b910-2078022cd675)

## Server_Node:

<div style="text-indent: 40px;">

The server node acts as the central hub within the system, responsible for managing and coordinating all processes. It communicates with the five individual processes through named pipes, which serve as inter-process communication channels. The server node's primary role is to receive data from one or more of these processes, process or analyze the information as required, and then distribute the results or relevant data back to the other processes. This centralized management ensures that the system operates efficiently and maintains synchronization between all interconnected components. Additionally, the server node's design allows for streamlined communication, enabling seamless data transfer and reducing potential conflicts or delays in the system's operation. 
</div>

![image](https://github.com/user-attachments/assets/3fe2d630-a278-44bc-af1c-3f375af7671b)

<div style="text-indent: 40px;">
Init will initialize the system as shown in the figure and create fifo will create the pipes if they do not exist and open will open those pipes. For select monitor we would discuss it in detail in the following:
</div>

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

![image](https://github.com/user-attachments/assets/6badc6bd-3e3b-465a-ab6f-e366325632ef)


This node is responsible for generating targets dynamically, handling signals for target generation, and maintaining communication with other processes through a FIFO (named pipe). Below is the detailed explanation:

### 1. Start

The program begins execution by clearing the log file and initializing the required resources.

---

### 2. Clear Log File

The program clears the log file (`log_target.txt`) to ensure a clean start for logging target generation activities during the session.

---

### 3. Initialize Program

The `init` function performs the following steps:

- **Checks if the FIFO file (`./pipes/targetgenerator`) exists:**
  - If it does not exist, it creates the FIFO.

- **Opens the FIFO in write mode.**

- **Writes the process ID (PID)** of the target generator program to the FIFO.

- **Registers the signal handler for SIGUSR1.**

---

### 4. Main Loop

After initialization, the program enters the main loop to perform periodic operations and wait for signals.

---

### 5. SIGUSR1 Signal Received?

The program waits for the **SIGUSR1** signal to initiate target generation. The signal handler performs the following:

- **Logs the receipt of the signal.**

- **Calls the `target_generator` function** to generate a new set of targets.

- **Sends the generated targets to the server** via the FIFO.

- If no signal is received, the program continues checking the watchdog timeout.

---

### 6. Generate Targets

The `target_generator` function:

- **Logs the start of target generation.**

- **Generates up to 10 targets** with random positions (`x, y`) and values:
  - Positions are even numbers within a defined range.
  - Each target is marked as active (`active = 1`).

- **Logs the generated targets** and writes them to the FIFO.

---

### 7. Check Watchdog Timeout

The program periodically checks if the watchdog timeout (4 seconds) has been reached. If the timeout is reached:

- **Updates the watchdog file** to signal that the process is still active.

- **Logs this activity** in the log file.

If the timeout has not been reached, the program continues the main loop.


## Obstcale_Generator_Node

![image](https://github.com/user-attachments/assets/6fbfbceb-c245-47ca-806f-ed64d8467812)

This node dynamically generates obstacles for a simulation, handles communication with other processes using named pipes (FIFO), and ensures process activity monitoring via a watchdog mechanism. The obstacles are periodically sent to the server so that the environment changes over time or when the obstacle generation key is pressed.

Below is a detailed explanation of the flowchart:

---

### 1. Start

The program begins execution, initializing required resources and configurations.

---

### 2. Initialize Program

The `init` function performs the following tasks:

- **Clears the log file (`log_obsticale.txt`)** to ensure a clean start for logging activities.

- **Checks if the FIFO file (`./pipes/obsticalegenerator`) exists:**
  - If it does not exist, the program creates the FIFO file.

- **Opens the FIFO in write mode** and writes the process ID (PID) of the obstacle generator to the FIFO so the server can use it later.

- **Registers a signal handler for SIGUSR1.**

---

### 3. Create and Open Pipes

- **Request Pipe (`PIPE_REQUEST_OBSTICALE`)**: Used to send generated obstacle data to the server.

- **Response Pipe (`PIPE_RESPONSE_OBSTICALE`)**: Used to receive responses from the server.

- If either pipe fails to open, the program logs the error and exits.

---

### 4. Pipes Opened Successfully?

- **Yes**: The program proceeds to register the signal handler for **SIGUSR1** and enters the main loop.
- **No**: Logs the error and terminates.

---

### 5. Register Signal Handler

The program registers a signal handler for **SIGUSR1**, which is used to trigger obstacle generation on-demand.

---

### 6. Generate Obstacles

The `obsticale_generator` function dynamically creates obstacles. The steps include:

- **Randomly generating positions (`x, y`)** for each obstacle:
  - Odd numbers are generated to ensure placement within a specific grid.
  - Example: `x = (rand() % 45) * 2 + 1` generates random odd positions between 1 and 89.

- **Assigning properties to each obstacle:**
  - **Size**: Fixed size (1x1) in this implementation.
  - **Active**: All obstacles are marked as active (`active = 1`).

- **Logging the generated obstacles** to the log file.

---

### 7. Send Obstacles to Server

Once obstacles are generated, they are sent to the server using the **request pipe (`PIPE_REQUEST_OBSTICALE`)**.

---

### 8. Watchdog Timeout?

The program periodically checks if the watchdog timeout (4 seconds) has been reached. If the timeout is reached:

- **Updates the watchdog file** to signal that the process is still active.

- **Logs this activity** in the log file.

If the timeout has not been reached, the program continues the loop.

---

### Purpose

The obstacle generator node ensures that the environment remains dynamic by periodically generating new obstacles and sending them to the server. It also enables on-demand obstacle updates triggered by external signals.


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

The **dynamic process** in the Drone Simulator, implemented in the `dynamic.c` file, simulates the drone's movement based on **user-applied forces** from the motors and **repulsive forces** from obstacles and geofence boundaries. The drone's **acceleration** and **velocity** are then calculated based on its **mass**, **viscosity** (air resistance), and the applied forces. This process ensures the drone behaves realistically by updating its position, velocity, and forces over time.

## Key Components

### 1. **Motor Force (Input Force)**

The **motor forces** (`fx`, `fy`) are directly controlled by user input, specifying the force applied by the drone's motors in the X and Y directions. These forces define the drone’s desired movement, including directional movements like **up**, **down**, **left**, **right**, and **diagonals**.

### 2. **Repulsive Force (Kathib’s Model)**

The **repulsive force** used for obstacle avoidance is based on **Kathib’s model**. The model applies a **repulsive force** when the drone is within a specified **perception radius** (`rho`) of an obstacle. The force increases as the drone gets closer to the obstacle, pushing the drone away to avoid collisions. The strength of this repulsive force is governed by the **repulsion strength coefficient** (`eta`).

The formula for the repulsive force is based on an inverse-square law:

```
F_rep = eta * (1 / distance - 1 / rho) * (1 / distance^2)
```

Where:
- **`distance`**: The distance between the drone and the obstacle.
- **`eta`**: The repulsion strength.
- **`rho`**: The perception radius, within which repulsive forces are applied.

### 3. **Viscosity of Air (`K`)**

The **viscous drag** force (air resistance) opposes the drone’s motion. It is proportional to the drone’s velocity and is calculated using the **viscosity coefficient** (`K`). This drag force slows the drone down as it moves through the air, simulating air resistance.

```
F_drag = -K * velocity
```

Where:
- **`K`**: The **viscous coefficient** that defines the air resistance.
- **`velocity`**: The drone's current velocity.

### 4. **Mass of the Drone (`M`)**

The **mass** of the drone directly affects its **acceleration**. The greater the mass, the less it accelerates for the same applied force. The acceleration is calculated using **Newton’s second law**:

```
a = F / M
```

Where:
- **`a`**: The **acceleration** of the drone.
- **`F`**: The total applied force (motor forces + repulsive forces).
- **`M`**: The mass of the drone.

### 5. **Acceleration and Velocity Update**

Once the **total forces** (motor forces + repulsive forces) are calculated, the drone’s **acceleration** is determined based on its mass. The **velocity** and **position** are then updated over time, with air resistance (viscosity) further slowing the drone's motion.

### 6. **Geofence Enforcement with Repulsive Effect (`X_max`, `Y_max`)**

The drone must remain within the **geofence boundaries**. If the drone approaches or exceeds these boundaries (`X_max`, `Y_max`), a **repulsive force** is applied to push the drone back within the limits, similar to how it avoids obstacles.

---

## Dynamic Process - `update_Drone_dynamics`

The `update_Drone_dynamics()` function is responsible for updating the drone's position, velocity, and acceleration based on user input and the forces applied. The function accounts for the **input forces**, **repulsive forces** from obstacles and geofence boundaries, and the **viscous drag** (air resistance).

### Key Steps:

1. **Force Calculation**:
   - The drone’s movement is controlled by **motor forces** (`fx`, `fy`), which are determined by user input.
   - The **repulsive forces** are calculated if the drone is near obstacles or the geofence boundary.
   - **Viscous drag** forces are calculated based on the drone’s velocity and air viscosity (`K`).

2. **Acceleration Calculation**:
   - The drone’s **acceleration** is calculated using the total applied forces divided by its mass (`M`):

   ```
   a = (F_motor + F_repulsive ) / M
   ```

3. **Velocity and Position Update**:
   - The drone’s **velocity** is updated based on the calculated **acceleration**.
   - The drone’s **position** is then updated using the new **velocity**.

---

## Example Code for Dynamic Update

Here’s an example of how the forces are applied and how the **acceleration** and **velocity** are computed:

```c
// Calculate the total force (motor forces + repulsive forces)
float total_fx = state->drone.fx + F_repulsive_x + F_geofence_x;
float total_fy = state->drone.fy + F_repulsive_y + F_geofence_y;

// Calculate drag forces based on velocity
float F_drag_x = -K * state->drone.vx;
float F_drag_y = -K * state->drone.vy;

// Calculate total forces in both X and Y directions
float total_force_x = total_fx + F_drag_x;
float total_force_y = total_fy + F_drag_y;

// Calculate acceleration using Newton’s second law: a = F / M
float ax = total_force_x / M;
float ay = total_force_y / M;

// Update velocity based on acceleration
state->drone.vx += ax * T;  // Update velocity in X direction
state->drone.vy += ay * T;  // Update velocity in Y direction

// Update position based on velocity
state->drone.x += state->drone.vx * T;  // Update position in X direction
state->drone.y += state->drone.vy * T;  // Update position in Y direction
```

In this example:
- **`total_fx`** and **`total_fy`** represent the forces applied by the motors and the repulsive forces from obstacles and the geofence.
- **`F_drag_x`** and **`F_drag_y`** are the drag forces due to viscosity.
- The **acceleration** is computed by dividing the total forces by the drone's **mass** (`M`), and this is used to update the drone’s **velocity** and **position**.

---

## Example Code for Repulsive and Geofence Forces

The drone applies **repulsive forces** for both obstacles and geofence boundaries:

```c
// Repulsive force for obstacles (Kathib’s Model)
if (distance_to_obstacle < rho) {
    float repulsive_force = eta * (1.0f / distance_to_obstacle - 1.0f / rho) / (distance_to_obstacle * distance_to_obstacle);
    state->drone.fx -= repulsive_force * (obstacle_x - state->drone.x); // X direction
    state->drone.fy -= repulsive_force * (obstacle_y - state->drone.y); // Y direction
}

// Geofence enforcement (Kathib’s Model)
if (state->drone.x > X_max) {
    float distance_to_boundary = state->drone.x - X_max;
    float F_geofence_x = eta * (1.0f / distance_to_boundary - 1.0f / X_max) / (distance_to_boundary * distance_to_boundary);
    state->drone.fx -= F_geofence_x; // Push the drone back within the X boundary
}

if (state->drone.y > Y_max) {
    float distance_to_boundary = state->drone.y - Y_max;
    float F_geofence_y = eta * (1.0f / distance_to_boundary - 1.0f / Y_max) / (distance_to_boundary * distance_to_boundary);
    state->drone.fy -= F_geofence_y; // Push the drone back within the Y boundary
}
```

---

## Parameter File - `parameters.txt`

The behavior of the drone is governed by several key parameters set in the **parameter file** (`parameters.txt`). These parameters define important aspects such as the drone's mass, viscosity, the force limits, and the perception radius for obstacle avoidance.

### Example Parameter File:

```
M       1.0   # Mass of the drone (kg)
K       0.5   # Viscous coefficient (N·s/m)
T       0.1   # Time step (s)
F_max   3.0   # Maximum force (N)
X_max   100.0 # Geofence X boundary (m)
Y_max   50.0  # Geofence Y boundary (m)
rho     3     # Obstacle perception radius (m)
eta     40    # Obstacle repulsion strength coefficient
zeta    0.5   # Target attraction strength coefficient
d_goal  10.0  # Distance threshold for full attraction (m)
```

### Key Parameters:

- **`M`**: Mass of the drone, affecting how it accelerates in response to applied forces.
- **`K`**: Viscous drag coefficient, simulating air resistance.
- **`T`**: Time step for the simulation, determining the frequency of updates.
- **`F_max`**: The maximum force the motors can apply.
- **`X_max`, `Y_max`**: The geofence boundaries, limiting the drone's movement.
- **`rho`**: The radius within which the drone perceives obstacles for collision avoidance.
- **`eta`**: The repulsion strength for obstacles and geofence enforcement.
- **`zeta`**: The strength of the attraction force for targets (not discussed here).
- **`d_goal`**: The distance at which the drone is fully attracted to a target.

---

## Conclusion

The **dynamic process** in `dynamic.c` models the drone's movement by applying **motor forces** and **repulsive forces** for obstacle avoidance and geofence enforcement. The drone’s **acceleration** and **velocity** are influenced by **viscosity**, **mass**, and the applied forces, ensuring realistic motion. The behavior of the drone can be easily adjusted through the **parameter file**, allowing for flexible simulation configurations.

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



https://github.com/user-attachments/assets/4ceccc65-b639-4c72-83a2-41cc41983df4



## Contact Information:

If you have any questions, suggestions, or feedback regarding these this project, please feel free to contact me at waleedelfieky@gmail.com

