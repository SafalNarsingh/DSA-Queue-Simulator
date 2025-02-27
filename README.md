# Queue Traffic Simulator

## Problem Description

A ***traffic junction*** connects two major roads, forming a central point where vehicles must choose
one of three alternative paths to continue (visual representation is shown in Figure 1). The traffic
management system must handle the following scenarios:
- #### Normal Condition:
    Vehicles at the junction are served equally from each lane. The system should ensure that vehicles are dispatched fairly.
- #### High-Priority Condition:
    If one of the roads (referred to as the priority road) accumulates more than 10 waiting vehicles, that road should be served first until the count drops below 5. Afterward, normal conditions resume. Lane `CL2` is considered as the lane having the highest priority.
  ![Path](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/2ce0dd60b09adaad7e4aa5f72f7907834768a023/imgs/path_visual.png)

## Installation 
To install and run the traffic simulator, follow these steps:
1. Clone the repository on your local environment:
```s
git clone https://github.com/SafalNarsingh/DSA-Queue-Simulator
```
2. Install SDL 2 library:
    - For Linux/Unix:
      ```s
      git clone https://github.com/libsdl-org/SDL.git -b SDL2
      cd SDL
      mkdir build
      cd build
      ../configure
      make
      sudo make install
      ```
    - For macOS:
      ```s
      mkdir build
      cd build 
      CC=/where/i/cloned/SDL/build-scripts/clang-fat.sh ../configure
      make
      ```
    - For Windows:
      You do not need to install SDL 2 on your computer. You can include the SDL 2 path in your respective local environment.
      
3. Build the project using:
   ```s
   gcc -o simulator simulator.c -I./src/include -L./src/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -pthread
   ```

4. Run the executable:
   ```s
   ./simulator
   ```

## Troubleshooting:
Make sure mingw and pthread, POSIX threads library for MinGW, are installed on your device.

## Dependencies:
- C
- SDL2
    
## Insights
#### Normal Lane:
- All lanes are normal lanes until explicitly declared as a priority. This lane will be served on normal conditions based on the average number of vehicles waiting on normal lanes.

![Vehicle Served at Once Equation](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/2ce0dd60b09adaad7e4aa5f72f7907834768a023/imgs/equation.png)  

Where, </br>
n: Total number of lanes (in this case it is 3 ⇒(BL2, CL3, DL4))  </br>
|Li|: Total number of vehicle waiting on ***ith lane*** </br>
|V|: Total number of vehicles served. </br>
</br>
It is hard to count the number of vehicles passing the junction. To solve this problem we have to estimate the time required for passing m number of vehicles. </br>
</br>
Total time of green light(state 2) = |V| ∗ t </br>
Where,</br>
T: Estimated time required to pass one vehicle</br>

#### Priority Lane
- This lane is a special lane where the waiting time should be low. When there are more than 5 vehicles waiting this lane should be served immediately after the running lighting condition ends.
- Special Case: For the vehicle less than 5, this lane also turn into the normal lane.

#### Traffic Lights
- For simplicity, there will be a total of four traffic lights each of which will instruct the vehicle of the opposite lane.
- Red Light: State 1 ⇒Stop
- Green Light: State 2 ⇒Go straight or turn
  
#### Preview

![Simulation GIF](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/2ce0dd60b09adaad7e4aa5f72f7907834768a023/imgs/Traffic%20Simulator%20GIF.gif)
</br></br></br>
![Example1](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/2ce0dd60b09adaad7e4aa5f72f7907834768a023/imgs/example1.png)
</br></br></br>
![Example2](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/2ce0dd60b09adaad7e4aa5f72f7907834768a023/imgs/example2.png)
</br></br></br>
![Vehicle Counter for lane A and C](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/2ce0dd60b09adaad7e4aa5f72f7907834768a023/imgs/laneStatus1.png)
![Vehicle Counter for lane B and D](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/2ce0dd60b09adaad7e4aa5f72f7907834768a023/imgs/laneStatus2.png)
</br>(Monitor the status of each lane to effectively track queue sizes)
</br></br>
#
Here is a brief of the code:

## Vehicle Generator

#### Overview
`Vehicle` is the main structure used for vehicle generation. It contains the attributes required for the vehicle to be generated like its position(x,y), its color, its lane, etc.
</br>
```C
typedef struct {
    char id[9];        
    int x, y;          
    char lane;        
    int sublane;       
    bool active;
    int direction;    
    int route_type;   
    char target_lane;  
    int target_sublane; 
    int choice;
    SDL_Color color;
} Vehicle;
```

</br>

```C
#define VEHICLE_SIZE 40
#define VEHICLE_LENGTH 60  
#define MAX_VEHICLES 500   
#define VEHICLE_SPEED 4
```
Constants defined for Vehicle Generation.

</br>

```C
Vehicle vehicles[MAX_VEHICLES];
```
`vehicles` has been declared as the structure variable.


</br></br>

Four  major functions have been implemented for rendering the vehicle on the screen.
```C
void drawVehicles(SDL_Renderer* renderer);                     //draws the vehicle 
void initVehicles();                                           //initializes the vehicle in not moving state
void spawnVehicle(const char* id, char lane, int sublane) ;    //positions the vehicle on the screen based on the lane and sublane
void updateVehicle();                                          //responsible for moving, turning and stopping of vehicle
```

</br>

with some helper functions to provide additional functionalities to the generated vehicle.
```C
void countVehiclesPerLane(int laneQueue[], int sublane);        //Counts the number of vehicles in sublane 2
```

## Traffic Generator
#### Overview
`TrafficLight` is implemented as the structure to define the state of the lights.

```C
typedef struct {
    bool green;
} TrafficLight;
```

</br>

```C
TrafficLight trafficLights[4]
```
`trafficLights` has be defined as the structure variable.

</br></br>
Major functions implemented for traffic light generation are:
```C
void initTrafficLights();                                         //Initialize the lights to red as default
void drawTrafficLights(SDL_Renderer* renderer)                    //Render the traffic lights
void* updateTrafficLights(void* arg);                             //Traffic lights function for normal priority lanes
void* updateTrafficLightsAdvanced(void* arg);                     //Traffic lights function for high-priority lanes
```

## Queue Implementation
#### Overview
In our queue implementation, we continuously monitor the number of vehicles waiting in each lane and prioritize dequeuing vehicles from lanes with higher vehicle counts, while simultaneously enqueuing vehicles from lanes at red lights. If the queue size of any lane ***exceeds 5 vehicles***, that lane will be dequeued first. However, if lane `Cl2` ***exceeds 5 vehicles***, it receives top priority and will be dequeued first until its count drops back below 5 vehicles.
</br>

For Queue implementation, `TrafficQueueu` structure has ben implemented.
```C
typedef struct {
    int vehicleIndices[MAX_QUEUE_SIZE]; // Stores indices of vehicles in queue
    int front;
    int rear;
    int size;
} TrafficQueue;
```

</br>

`laneQueues` has been defined as the structure variable.
```C
TrafficQueue laneQueues[NUM_LANES];                            // Create a queue for each lane to track vehicles waiting at lights
```

</br></br>
Functions used for traffic queue management has been listed below:
```C
void initQueue(TrafficQueue* queue);                             //Initialize a new queue
bool isQueueEmpty(TrafficQueue* queue);                          //Check if queue is empty
bool isQueueFull(TrafficQueue* queue);                           //Check if queue is full
bool enqueue(TrafficQueue* queue, int vehicleIndex)              //Add a vehicle index to the queue
int dequeue(TrafficQueue* queue;                                 //Remove a vehicle index from the queue
int peek(TrafficQueue* queue);                                   //Get front of queue without removing
void updateTrafficQueues()                                       //Enqueues or dequeues the traffic and size calculation
```

## Issues 
- CPU usage needs to be optimized.

</br>

> Last updated by Safal Narshing Shrestha on Feb 26, 2025, 23:53. </br> Copyright © 2025 Safal Narshing Shrestha. All rights reserved.
## The end.
