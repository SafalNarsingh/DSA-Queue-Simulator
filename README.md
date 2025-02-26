# Queue Traffic Simulator

## Problem Description

A ***traffic junction*** connects two major roads, forming a central point where vehicles must choose
one of three alternative paths to continue (visual representation is shown in Figure 1). The traffic
management system must handle the following scenarios:
- #### Normal Condition:
    Vehicles at the junction are served equally from each lane. The system should ensure that vehicles are dispatched fairly.
- #### High-Priority Condition:
    If one of the roads (referred to as the priority road) accumulates more than 10 waiting vehicles, that road should be served first until the count drops below 5. Afterward, normal conditions resume.
  ![Path](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/d127f7b16f1197517f2fa9790130cfd8aac52990/imgs/path_visual.png)

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
    
## Insights
#### Normal Lane:
All lanes are normal lanes until explicitly declared as a priority. This lane will be served on normal
conditions based on the average number of vehicles waiting on normal lanes.

![Vehicle Served at Once Equation]()  

Where,
n: Total number of lanes (in this case it is 3 ⇒(BL2, CL3, DL4))
|Li|: Total number of vehicle waiting on ***ith lane***
|V|: Total number of vehicles served.
It is hard to count the number of vehicles passing the junction. To solve this problem we have to
estimate the time required for passing m number of vehicles.
Total time of green light(state 2) = |V| ∗ t

Where,
T: Estimated time required to pass one vehicle


#### Preview

![Road Image](https://github.com/SafalNarsingh/DSA-Queue-Simulator/blob/d127f7b16f1197517f2fa9790130cfd8aac52990/imgs/road.png)
![Example1]()
![Example2]()

Here are some details of the Code:

