#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h> 

#define WINDOW_WIDTH 1280
#define MAIN_FONT "DejaVuSans.ttf"
#define WINDOW_HEIGHT 720
#define SCALE 1
#define ROAD_WIDTH 210  
#define LANE_WIDTH 70   
#define VEHICLE_SIZE 40
#define VEHICLE_LENGTH 60  
#define MAX_VEHICLES 200   
#define VEHICLE_SPEED 3
#define LEFT_TURN 1
#define STRAIGHT 2
#define RIGHT_TURN 3
#define STOP_DISTANCE 175 // Distance from traffic light where vehicles should stop
#define MAX_QUEUE_SIZE 100 // Maximum size for our traffic queues
#define NUM_LANES 4 // A, B, C, D lanes

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
    SDL_Color color;
} Vehicle;

Vehicle vehicles[MAX_VEHICLES];
SDL_mutex* vehicleMutex;


typedef struct{
    int currentLight;
    int nextLight;
} SharedData;



typedef struct {
    bool green;
} TrafficLight;

TrafficLight trafficLights[4];


// Queue implementation for traffic management
typedef struct {
    int vehicleIndices[MAX_QUEUE_SIZE]; // Stores indices of vehicles in queue
    int front;
    int rear;
    int size;
} TrafficQueue;

TrafficQueue laneQueues[NUM_LANES];

// Initialize a new queue
void initQueue(TrafficQueue* queue) {
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
}

void initTrafficLights() {
    for (int i = 0; i < 4; i++) {
        trafficLights[i].green = false;
        initQueue(&laneQueues[i]); // Initialize all lane queues
    }
}
// Check if queue is empty
bool isQueueEmpty(TrafficQueue* queue) {
    return queue->size == 0;
}

// Check if queue is full
bool isQueueFull(TrafficQueue* queue) {
    return queue->size == MAX_QUEUE_SIZE;
}

// Add a vehicle index to the queue
bool enqueue(TrafficQueue* queue, int vehicleIndex) {
    if (isQueueFull(queue)) {
        return false;
    }
    
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->vehicleIndices[queue->rear] = vehicleIndex;
    queue->size++;
    return true;
}

// Remove a vehicle index from the queue
int dequeue(TrafficQueue* queue) {
    if (isQueueEmpty(queue)) {
        return -1; // Error: Empty queue
    }
    
    int vehicleIndex = queue->vehicleIndices[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->size--;
    return vehicleIndex;
}

// Get front of queue without removing
int peek(TrafficQueue* queue) {
    if (isQueueEmpty(queue)) {
        return -1; // Error: Empty queue
    }
    
    return queue->vehicleIndices[queue->front];
}

// Create a queue for each lane to track vehicles waiting at lights
TrafficQueue laneQueues[NUM_LANES];

TrafficLight trafficLights[4]; // One for each lane

// Count the number of vehicles in each lane
void countVehiclesPerLane(int laneQueue[], int sublane) {
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].active && vehicles[i].sublane == 2) {
            switch (vehicles[i].lane) {
                case 'A':
                    laneQueue[0]++;
                    break;
                case 'B':
                    laneQueue[1]++;
                    break;
                case 'C':
                    laneQueue[2]++;
                    break;
                case 'D':
                    laneQueue[3]++;
                    break;
            }
        }
    }
}

// Update traffic lights dynamically
void* updateTrafficLights(void* arg) {
    while (1) {
        SDL_LockMutex(vehicleMutex);
        
        int laneCounts[4] = {0};
        
        // Count vehicles in sublane 2 at intersections A, B, C, and D
        countVehiclesPerLane(laneCounts, 2);

        // Default priority lane
        int priorityLane = -1; 
        for (int i = 0; i < 4; i++) {
            if (laneCounts[i] >= 3) { 
                priorityLane = i; // Prioritize lanes with 3+ vehicles in sublane 2
                break;
            }
        }

        // Set all traffic lights for sublane 2 to red
        for (int i = 0; i < 4; i++) {
            trafficLights[i].green = false;
        }
        if (priorityLane != -1) {
            trafficLights[priorityLane].green = true; // Enable priority lane for sublane 2
        }

        SDL_UnlockMutex(vehicleMutex);

        sleep(5); // Give time for vehicles to clear
    }
    return NULL;
}


void updateTrafficQueues() {
    SDL_LockMutex(vehicleMutex);
    
    // Clear all queues first (we'll rebuild them every update)
    for (int i = 0; i < NUM_LANES; i++) {
        initQueue(&laneQueues[i]);
    }
    
    // For each active vehicle, check if it's approaching an intersection
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) continue;
        
        // Only consider vehicles in sublane 2 (straight lane)
        if (vehicles[i].sublane != 2) continue;
        
        bool isApproachingIntersection = false;
        int laneIndex = -1;
        
        switch (vehicles[i].lane) {
            case 'A':
                // Vehicle approaching from left - increase detection range
                if (vehicles[i].x >= (WINDOW_WIDTH / 2 - STOP_DISTANCE * 3) && 
                    vehicles[i].x < (WINDOW_WIDTH / 2)) {
                    isApproachingIntersection = true;
                    laneIndex = 0;
                }
                break;
                
            case 'B':
                // Vehicle approaching from right - increase detection range
                if (vehicles[i].x <= (WINDOW_WIDTH / 2 + STOP_DISTANCE * 3) && 
                    vehicles[i].x > (WINDOW_WIDTH / 2)) {
                    isApproachingIntersection = true;
                    laneIndex = 1;
                }
                break;
                
            case 'C':
                // Vehicle approaching from top - increase detection range
                if (vehicles[i].y >= (WINDOW_HEIGHT / 2 - STOP_DISTANCE * 3) && 
                    vehicles[i].y < (WINDOW_HEIGHT / 2)) {
                    isApproachingIntersection = true;
                    laneIndex = 2;
                }
                break;
                
            case 'D':
                // Vehicle approaching from bottom - increase detection range
                if (vehicles[i].y <= (WINDOW_HEIGHT / 2 + STOP_DISTANCE * 3) && 
                    vehicles[i].y > (WINDOW_HEIGHT / 2)) {
                    isApproachingIntersection = true;
                    laneIndex = 3;
                }
                break;
        }
        
        if (isApproachingIntersection && laneIndex != -1) {
            enqueue(&laneQueues[laneIndex], i);
        }
    }
    
    SDL_UnlockMutex(vehicleMutex);

    // Debug: Print queue sizes
    printf("Queue sizes after update: A:%d, B:%d, C:%d, D:%d\n", 
           laneQueues[0].size, laneQueues[1].size, 
           laneQueues[2].size, laneQueues[3].size);
}

void* updateTrafficLightsWithQueues(void* arg) {
    int currentPriorityLane = -1;
    int greenLightDuration = 0;
    
    while (1) {


        updateTrafficQueues();
        sleep(3); // Check every 2 seconds
        
        // Get queue sizes for debugging
        printf("Queue sizes: A:%d, B:%d, C:%d, D:%d\n", 
               laneQueues[0].size, laneQueues[1].size, 
               laneQueues[2].size, laneQueues[3].size);
        
        // Check if we need to change the priority lane
        bool needNewPriority = false;
        
        // If current priority lane's queue is empty
        if (currentPriorityLane == -1 || laneQueues[currentPriorityLane].size == 0) {
            needNewPriority = true;
        }
        
        // If green light has been on for a very long time (safety measure)
        if (greenLightDuration >= 10) {
            needNewPriority = true;
        }
        
        // If we need a new priority lane
        if (needNewPriority) {
            // Find lane with at least 10 vehicles waiting
            int maxQueueSize = 0;
            int maxQueueLane = -1;
            
            for (int i = 0; i < NUM_LANES; i++) {
                // Only consider lanes with at least 10 vehicles
                if (laneQueues[i].size >= 10 && laneQueues[i].size > maxQueueSize) {
                    maxQueueSize = laneQueues[i].size;
                    maxQueueLane = i;
                }
            }
            
            // Only switch if there's a lane with at least 10 vehicles waiting
            if (maxQueueLane != -1) {
                // Set all lights to red
                SDL_LockMutex(vehicleMutex);
                for (int i = 0; i < 4; i++) {
                    trafficLights[i].green = false;
                }
                
                // Set new priority lane to green
                trafficLights[maxQueueLane].green = true;
                currentPriorityLane = maxQueueLane;
                SDL_UnlockMutex(vehicleMutex);
                greenLightDuration = 0;
                
                printf("Switching green light to lane %c (queue size: %d)\n", 
                       'A' + maxQueueLane, maxQueueSize);
            }
        } else {
            greenLightDuration++;
        }
        
       
    }
    return NULL;
}

// Function to determine route type based on source and target lanes
int getRouteType(char source_lane, int source_sublane, char target_lane, int target_sublane) {
    if (source_lane == 'A') {
        if (source_sublane == 1 && target_lane == 'C') return LEFT_TURN;
        if (source_sublane == 2 && target_lane == 'B') return STRAIGHT;
        if (source_sublane == 3 && target_lane == 'D') return RIGHT_TURN;
    }
    // Add similar logic for other lanes
    // For lane B
    if (source_lane == 'B') {
        if (source_sublane == 1 && target_lane == 'D') return LEFT_TURN;
        if (source_sublane == 2 && target_lane == 'A') return STRAIGHT;
        if (source_sublane == 3 && target_lane == 'C') return RIGHT_TURN;
    }
    // For lane C
    if (source_lane == 'C') {
        if (source_sublane == 1 && target_lane == 'B') return LEFT_TURN;
        if (source_sublane == 2 && target_lane == 'D') return STRAIGHT;
        if (source_sublane == 3 && target_lane == 'A') return RIGHT_TURN;
    }
    // For lane D
    if (source_lane == 'D') {
        if (source_sublane == 1 && target_lane == 'A') return LEFT_TURN;
        if (source_sublane == 2 && target_lane == 'C') return STRAIGHT;
        if (source_sublane == 3 && target_lane == 'B') return RIGHT_TURN;
    }
    return STRAIGHT; // Default case
}


// Convert lane letter and number to screen coordinates
void getLanePosition(char lane, int sublane, int* x, int* y) {
    int center_x = WINDOW_WIDTH / 2;
    int center_y = WINDOW_HEIGHT / 2;

    switch (lane) {
        case 'A': // Left side lanes (moving right)
            *x = 0;  // Start from the left
            *y = center_y - ROAD_WIDTH / 2 + (sublane - 1) * LANE_WIDTH + LANE_WIDTH / 2; // Centered
            break;

        case 'B': // Right side lanes (moving left)
            *x = WINDOW_WIDTH; // Start from the right
            *y = center_y + ROAD_WIDTH / 2 - (sublane - 1) * LANE_WIDTH - LANE_WIDTH / 2; // Centered
            break;

        case 'C': // Top side lanes (moving down)
            *x = center_x - ROAD_WIDTH / 2 + (sublane - 1) * LANE_WIDTH + LANE_WIDTH / 2; // Centered
            *y = 0; // Start from the top
            break;

        case 'D': // Bottom side lanes (moving up)
            *x = center_x + ROAD_WIDTH / 2 - (sublane - 1) * LANE_WIDTH - LANE_WIDTH / 2; // Centered
            *y = WINDOW_HEIGHT; // Start from the bottom
            break;
    }
}

void initVehicles() {
    for (int i = 0; i < MAX_VEHICLES; i++) {
        vehicles[i].active = false;
    }
}

int getDirection(char lane) {
    return (lane == 'A' || lane == 'C') ? 1 : -1;
}

void spawnVehicle(const char* id, char lane, int sublane) {
    // Prevent spawning in Lane A, Sublane 3
    if (lane == 'A' && sublane == 3) {
        return; // Skip this vehicle
    }

    SDL_LockMutex(vehicleMutex);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) {
            vehicles[i].active = true;
            snprintf(vehicles[i].id, 9, "%s", id);
            vehicles[i].lane = lane;
            vehicles[i].sublane = sublane;
            vehicles[i].direction = (lane == 'A' || lane == 'C') ? 1 : -1;
            getLanePosition(lane, sublane, &vehicles[i].x, &vehicles[i].y);

            // Initialize the color attribute
            vehicles[i].color = (SDL_Color){rand() % 256, rand() % 256, rand() % 256, 255};

            printf("Spawned Vehicle: %s at lane %c, sublane %d\n", vehicles[i].id, lane, sublane);
            break;
        }
    }
    SDL_UnlockMutex(vehicleMutex);
}

void* generateVehicles(void* arg) {
    srand(time(NULL));
    char lanes[] = {'A', 'B', 'C', 'D'};

    while (1) {
        int laneIndex = rand() % 4;
        int sublane = (rand() % 3) + 1;

        // Prevent spawning in `A3` and `D1`
        if ((lanes[laneIndex] == 'A' && sublane == 3) ||(lanes[laneIndex] == 'B' && sublane == 3) || (lanes[laneIndex] == 'D' && sublane == 1) || (lanes[laneIndex] == 'C' && sublane == 1)) {
            continue; // Skip this iteration
        }

        char vehicleID[9];
        snprintf(vehicleID, 9, "V%03d", rand() % 1000);

        spawnVehicle(vehicleID, lanes[laneIndex], sublane);
        sleep(2);
    }
    return NULL;
}

void updateVehicles() {
    SDL_LockMutex(vehicleMutex);
    int choice = rand() % 2;
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) continue;

        bool canMove = true;

        // Check if there is a vehicle ahead in the same lane and sublane
        for (int j = 0; j < MAX_VEHICLES; j++) {
            if (i != j && vehicles[j].active && vehicles[j].lane == vehicles[i].lane && vehicles[j].sublane == vehicles[i].sublane) {
                if (vehicles[i].lane == 'A' && vehicles[i].x < vehicles[j].x && vehicles[j].x - vehicles[i].x < VEHICLE_LENGTH + 10) {
                    canMove = false;
                    break;
                }
                if (vehicles[i].lane == 'B' && vehicles[i].x > vehicles[j].x && vehicles[i].x - vehicles[j].x < VEHICLE_LENGTH + 10) {
                    canMove = false;
                    break;
                }
                if (vehicles[i].lane == 'C' && vehicles[i].y < vehicles[j].y && vehicles[j].y - vehicles[i].y < VEHICLE_LENGTH + 10) {
                    canMove = false;
                    break;
                }
                if (vehicles[i].lane == 'D' && vehicles[i].y > vehicles[j].y && vehicles[i].y - vehicles[j].y < VEHICLE_LENGTH + 10) {
                    canMove = false;
                    break;
                }
            }
        }

        if (!canMove) continue;

        switch (vehicles[i].lane) {
            case 'A': 
                if (vehicles[i].sublane == 2 && !trafficLights[0].green && vehicles[i].x >= (WINDOW_WIDTH / 2 - STOP_DISTANCE)) {
                    continue; // Stop if light is red and vehicle is close enough
                }
                vehicles[i].x += VEHICLE_SPEED; // Move right

                if (vehicles[i].sublane == 2 && vehicles[i].x >= WINDOW_WIDTH / 2 - 75) {
                    vehicles[i].y -=  VEHICLE_SPEED;  // Move smoothly up or down
                    if (vehicles[i].y <= WINDOW_HEIGHT / 2 - 72 ) {
                        vehicles[i].lane = 'A'; // Change the lane to either C3 or A1
                        vehicles[i].sublane =  1;
                    }
                }
                // **A1 should turn left into D1 smoothly**
                if (vehicles[i].sublane == 1 && vehicles[i].x <= WINDOW_WIDTH / 2 - 50 && vehicles[i].x >= WINDOW_WIDTH / 2 - 75) {
                    printf("Turning left: Vehicle %s from A1 to D1\n", vehicles[i].id);
                    
                    // Start moving upward instead of continuing right
                    vehicles[i].y -= VEHICLE_SPEED; 
                    
                    // If vehicle has reached the middle, switch lanes
                    if (vehicles[i].y <= WINDOW_HEIGHT / 2 - 75 ) {
                        vehicles[i].lane = 'D';
                        vehicles[i].sublane = 1;
                        vehicles[i].direction = 1; // Move down in D1
                    }
                }
                break;

            case 'B': 
                if (vehicles[i].sublane == 2 && !trafficLights[1].green && vehicles[i].x <= (WINDOW_WIDTH / 2 + STOP_DISTANCE)) {
                    continue; // Stop if light is red and vehicle is close enough
                }
                vehicles[i].x -= VEHICLE_SPEED; // Move left
            
                if (vehicles[i].sublane == 2 && vehicles[i].x <= WINDOW_WIDTH / 2 + 75) {
                    vehicles[i].y += (vehicles[i].direction == 1) ? -VEHICLE_SPEED : VEHICLE_SPEED; // Move smoothly up or down
                    if (vehicles[i].y <= WINDOW_HEIGHT / 2 - 75 || vehicles[i].y >= WINDOW_HEIGHT / 2 + 75) {
                        vehicles[i].lane = 'B'; // Change the lane to either B3 or D1
                        vehicles[i].sublane = 3;
                    }
                }
                
                // **B1 should turn left into C1 smoothly**
                if (vehicles[i].sublane == 1 && vehicles[i].x <= WINDOW_WIDTH / 2 + 75) {
                    printf("Turning left: Vehicle %s from B1 to C1\n", vehicles[i].id);
                    
                    // Start moving downward instead of continuing left
                    vehicles[i].y += VEHICLE_SPEED;                     

                    // If vehicle has reached the middle, switch lanes
                    if (vehicles[i].y >= WINDOW_HEIGHT / 2) {
                        vehicles[i].lane = 'C';
                        vehicles[i].sublane = 1;
                        vehicles[i].direction = 1; // Move down in C1
                    }
                }
                break;

            case 'C': 
                if (vehicles[i].sublane == 2 && !trafficLights[2].green && vehicles[i].y >= (WINDOW_HEIGHT / 2 - STOP_DISTANCE)) {
                    continue; // Stop if light is red and vehicle is close enough
                }
                vehicles[i].y += VEHICLE_SPEED; // Move down

                if (vehicles[i].sublane == 2 && vehicles[i].y >= WINDOW_HEIGHT / 2  ) {
                    vehicles[i].x -=  VEHICLE_SPEED; // Move smoothly left 
                    if (vehicles[i].x <= WINDOW_WIDTH / 2 - 75) {
                        vehicles[i].lane = 'B'; // Change the lane to either C3 or B3
                        vehicles[i].sublane = 3;
                    }
                }
                // **C3 should turn left into A3 smoothly**
                if (vehicles[i].sublane == 3 && vehicles[i].y >= WINDOW_HEIGHT / 2 - 75) {
                    printf("Turning left: Vehicle %s from C3 to A3\n", vehicles[i].id);
                    
                    // Start moving right instead of continuing down
                    vehicles[i].x += VEHICLE_SPEED; 

                    // If vehicle has reached the middle, switch lanes
                    if (vehicles[i].x >= WINDOW_WIDTH / 2) {
                        vehicles[i].lane = 'A';
                        vehicles[i].sublane = 3;
                        vehicles[i].direction = 1; // Move right in A3
                    }
                }
                break;

            case 'D': 
                if (vehicles[i].sublane == 2 && !trafficLights[3].green && vehicles[i].y <= (WINDOW_HEIGHT / 2 + STOP_DISTANCE)) {
                    continue; // Stop if light is red and vehicle is close enough
                }
                vehicles[i].y -= VEHICLE_SPEED; // Move up

                if (vehicles[i].sublane == 2 && vehicles[i].y <= WINDOW_HEIGHT / 2 ) {
                    vehicles[i].x += -VEHICLE_SPEED; // Move smoothly left  
                    if (vehicles[i].x >= WINDOW_WIDTH / 2 + 75 || vehicles[i].x <= WINDOW_WIDTH / 2 - 75) {
                        vehicles[i].lane = 'D'; // Change the lane either D1 or A1
                        vehicles[i].sublane =  1;
                    }
                }
                // **D3 should turn left into B3 smoothly**
                if (vehicles[i].sublane == 3 && vehicles[i].y <= WINDOW_HEIGHT / 2 + 75) {
                    printf("Turning left: Vehicle %s from D3 to B3\n", vehicles[i].id);
                    
                    // Start moving left instead of continuing up
                    vehicles[i].x -= VEHICLE_SPEED; 

                    // If vehicle has reached the middle, switch lanes
                    if (vehicles[i].x <= WINDOW_WIDTH / 2 - 75) {
                        vehicles[i].lane = 'B';
                        vehicles[i].sublane = 3;
                        vehicles[i].direction = -1; // Move left in B3
                    }
                }
                break;
        }
    }
    SDL_UnlockMutex(vehicleMutex);
}

void drawTrafficLights(SDL_Renderer* renderer) {
    int center_x = WINDOW_WIDTH / 2;
    int center_y = WINDOW_HEIGHT / 2;
    
    SDL_Rect lights[4] = {
        { center_x - ROAD_WIDTH / 2 - 30, center_y - 20, 20, 40 }, // A ->
        { center_x + ROAD_WIDTH / 2 , center_y - 15 , 20, 40 }, // B |
        { center_x - 12, center_y -ROAD_WIDTH / 2 - 5, 40, 20 }, // C ^
        { center_x - 12, center_y + ROAD_WIDTH / 2 + 10, 40, 20 }  // D <-
    };
    
    for (int i = 0; i < 4; i++) {
        if (trafficLights[i].green)
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        else
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red

        SDL_RenderFillRect(renderer, &lights[i]);
    }
}


// Draw queue visualization
void drawQueueVisualization(SDL_Renderer* renderer) {
    // Display queue sizes as text
    char queueText[4][20];
    for (int i = 0; i < 4; i++) {
        sprintf(queueText[i], "Lane %c: %d", 'A' + i, laneQueues[i].size);
    }
    
    // Draw small boxes to represent queue size
    // For Lane A (left)
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_Rect queueBoxA = { 50, 50, 120, 25 };
    SDL_RenderFillRect(renderer, &queueBoxA);
    
    // For Lane B (right)
    SDL_Rect queueBoxB = { WINDOW_WIDTH - 170, 50, 120, 25 };
    SDL_RenderFillRect(renderer, &queueBoxB);
    
    // For Lane C (top)
    SDL_Rect queueBoxC = { 50, 80, 120, 25 };
    SDL_RenderFillRect(renderer, &queueBoxC);
    
    // For Lane D (bottom)
    SDL_Rect queueBoxD = { WINDOW_WIDTH - 170, 80, 120, 25 };
    SDL_RenderFillRect(renderer, &queueBoxD);
    
    // A line of differently colored boxes could represent vehicles in the queue
    // Left side for Lane A
    SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
    for (int i = 0; i < laneQueues[0].size && i < 10; i++) {
        SDL_Rect vBox = { 55 + i * 12, 55, 10, 15 };
        SDL_RenderFillRect(renderer, &vBox);
    }
    
    // Right side for Lane B
    SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
    for (int i = 0; i < laneQueues[1].size && i < 10; i++) {
        SDL_Rect vBox = { WINDOW_WIDTH - 165 + i * 12, 55, 10, 15 };
        SDL_RenderFillRect(renderer, &vBox);
    }
    
    // Top for Lane C
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
    for (int i = 0; i < laneQueues[2].size && i < 10; i++) {
        SDL_Rect vBox = { 55 + i * 12, 85, 10, 15 };
        SDL_RenderFillRect(renderer, &vBox);
    }
    
    // Bottom for Lane D
    SDL_SetRenderDrawColor(renderer, 255, 255, 100, 255);
    for (int i = 0; i < laneQueues[3].size && i < 10; i++) {
        SDL_Rect vBox = { WINDOW_WIDTH - 165 + i * 12, 85, 10, 15 };
        SDL_RenderFillRect(renderer, &vBox);
    }
}

// Update the drawVehicles function in simulator.c:void drawVehicles(SDL_Renderer* renderer) {
void drawVehicles(SDL_Renderer* renderer) {
    SDL_LockMutex(vehicleMutex);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) continue;

        // Use the color attribute
        SDL_SetRenderDrawColor(renderer, vehicles[i].color.r, vehicles[i].color.g, vehicles[i].color.b, vehicles[i].color.a);

        SDL_Rect rect;

        if (vehicles[i].lane == 'A' || vehicles[i].lane == 'B') { 
            // Vehicles moving horizontally (left/right)
            rect = (SDL_Rect){
                vehicles[i].x - VEHICLE_LENGTH / 2, 
                vehicles[i].y - VEHICLE_SIZE / 2, 
                VEHICLE_LENGTH,
                VEHICLE_SIZE
            };
        } else {  
            // Vehicles moving vertically (up/down)
            rect = (SDL_Rect){
                vehicles[i].x - VEHICLE_SIZE / 2 + 5, 
                vehicles[i].y - VEHICLE_LENGTH / 2, 
                VEHICLE_SIZE,
                VEHICLE_LENGTH
            };
        }

        SDL_RenderFillRect(renderer, &rect);
    }
    SDL_UnlockMutex(vehicleMutex);
}


void* readAndParseFile(void* arg) {
    printf("Reading vehicle data...\n");
    while(1) {
        FILE* file = fopen("vehicles.data", "r");
        if (!file) {
            perror("Error opening file");
            sleep(1);
            continue;
        }

        char line[20];
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;
            char* vehicleNumber = strtok(line, ":");
            char* lane = strtok(NULL, ":");
            char* sublane = strtok(NULL, ":");

            if (vehicleNumber && lane && sublane) {
                printf("Read vehicle: %s, Lane: %s, Sublane: %s\n", vehicleNumber, lane, sublane);
                spawnVehicle(vehicleNumber, lane[0], atoi(sublane));
            }
        }
        fclose(file);
        sleep(1);
    }
    return NULL;
}

const SDL_Color WHITE = {255, 255, 255, 255};
const SDL_Color GRAY = {30, 30, 30, 1};
const SDL_Color YELLOW = {250, 250, 0, 255};


const char* VEHICLE_FILE = "vehicles.data";


// Function declarations
bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font);
void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y);
void refreshLight(SDL_Renderer *renderer, SharedData* sharedData);
void* chequeQueue(void* arg);
void* readAndParseFile(void* arg);
void* mainLoop(void* arg);


void printMessageHelper(const char* message, int count) {
    for (int i = 0; i < count; i++) printf("%s\n", message);
}
int main(int argc, char *argv[]) {
    pthread_t vehicleThread, trafficThread, fileThread;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    TTF_Font* font = NULL;
    
    // Initialize SDL and create window and renderer
    if (!initializeSDL(&window, &renderer)) {
        return -1;
    }
    
    // Load font
    font = TTF_OpenFont(MAIN_FONT, 24);
    if (!font) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    
    // Initialize vehicle system and mutex
    vehicleMutex = SDL_CreateMutex();
    if (!vehicleMutex) {
        SDL_Log("Failed to create mutex: %s", SDL_GetError());
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    
    initVehicles();
    initTrafficLights();
    
    // Create threads
    if (pthread_create(&vehicleThread, NULL, generateVehicles, NULL) != 0) {
        SDL_Log("Failed to create vehicle thread");
        SDL_DestroyMutex(vehicleMutex);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    
    if (pthread_create(&trafficThread, NULL, updateTrafficLightsWithQueues, NULL) != 0) {
        SDL_Log("Failed to create traffic light thread");
        // Cancel the vehicle thread
        pthread_cancel(vehicleThread);
        SDL_DestroyMutex(vehicleMutex);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    
    if (pthread_create(&fileThread, NULL, readAndParseFile, NULL) != 0) {
        SDL_Log("Failed to create file parsing thread");
        // Cancel the other threads
        pthread_cancel(vehicleThread);
        pthread_cancel(trafficThread);
        SDL_DestroyMutex(vehicleMutex);
        TTF_CloseFont(font);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }
    
    // Main application loop
    bool running = true;
    Uint32 lastFrameTime = SDL_GetTicks();
    
    while (running) {
        // Handle SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            // Add any other event handling here as needed
        }
        
        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastFrameTime) / 1000.0f;
        lastFrameTime = currentTime;
        
        // Update simulation
        updateVehicles();
        updateTrafficQueues();
        
        // Render frame
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderClear(renderer);
        
        drawRoadsAndLane(renderer, font);
        drawTrafficLights(renderer);
        drawVehicles(renderer);
        drawQueueVisualization(renderer);
        
        SDL_RenderPresent(renderer);
        
        // Cap the frame rate
        Uint32 frameTime = SDL_GetTicks() - currentTime;
        if (frameTime < 16) {
            SDL_Delay(16 - frameTime);  // Target ~60 FPS
        }
    }
    
    // Cleanup and shutdown
    pthread_cancel(vehicleThread);
    pthread_cancel(trafficThread);
    pthread_cancel(fileThread);
    
    SDL_DestroyMutex(vehicleMutex);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return 0;
}

// Modified main loop
void* mainLoop(void* arg) {
    SDL_Renderer* renderer = (SDL_Renderer*)arg;
    Uint32 lastFrame = SDL_GetTicks();

    while (1) {
        Uint32 currentFrame = SDL_GetTicks();
        float deltaTime = (currentFrame - lastFrame) / 1000.0f;
        lastFrame = currentFrame;

        updateVehicles();  

        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Set background color to dark gray
        SDL_RenderClear(renderer);
        
        drawRoadsAndLane(renderer, NULL);
        drawTrafficLights(renderer);
        drawVehicles(renderer);

        SDL_RenderPresent(renderer);

        SDL_Delay(16); // Maintain ~60 FPS
    }
    return NULL;
}

bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return false;
    }
    // font init
    if (TTF_Init() < 0) {
        SDL_Log("SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        return false;
    }


    *window = SDL_CreateWindow("Traffic Simulator",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               WINDOW_WIDTH*SCALE, WINDOW_HEIGHT*SCALE,
                               SDL_WINDOW_SHOWN);
    if (!*window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    // if you have high resolution monitor 2K or 4K then scale
    SDL_RenderSetScale(*renderer, SCALE, SCALE);

    if (!*renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(*window);
        TTF_Quit();
        SDL_Quit();
        return false;
    }

    return true;
}


void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}


void drawArrwow(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, int x3, int y3) {
    // Sort vertices by ascending Y (bubble sort approach)
    if (y1 > y2) { swap(&y1, &y2); swap(&x1, &x2); }
    if (y1 > y3) { swap(&y1, &y3); swap(&x1, &x3); }
    if (y2 > y3) { swap(&y2, &y3); swap(&x2, &x3); }

    // Compute slopes
    float dx1 = (y2 - y1) ? (float)(x2 - x1) / (y2 - y1) : 0;
    float dx2 = (y3 - y1) ? (float)(x3 - x1) / (y3 - y1) : 0;
    float dx3 = (y3 - y2) ? (float)(x3 - x2) / (y3 - y2) : 0;

    float sx1 = x1, sx2 = x1;

    // Fill first part (top to middle)
    for (int y = y1; y < y2; y++) {
        SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
        sx1 += dx1;
        sx2 += dx2;
    }

    sx1 = x2;

    // Fill second part (middle to bottom)
    for (int y = y2; y <= y3; y++) {
        SDL_RenderDrawLine(renderer, (int)sx1, y, (int)sx2, y);
        sx1 += dx3;
        sx2 += dx2;
    }
}





void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font) {
    // Set road color
    SDL_SetRenderDrawColor(renderer, GRAY.r, GRAY.g, GRAY.b, GRAY.a);

    // Draw intersection
    SDL_Rect intersection = {
        (WINDOW_WIDTH - ROAD_WIDTH) / 2,
        (WINDOW_HEIGHT - ROAD_WIDTH) / 2,
        ROAD_WIDTH,
        ROAD_WIDTH
    };
    SDL_RenderFillRect(renderer, &intersection);

    // Draw horizontal road
    SDL_Rect horizontalRoad = { 0, (WINDOW_HEIGHT - ROAD_WIDTH) / 2, WINDOW_WIDTH, ROAD_WIDTH };
    SDL_RenderFillRect(renderer, &horizontalRoad);

    // Draw vertical road
    SDL_Rect verticalRoad = { (WINDOW_WIDTH - ROAD_WIDTH) / 2, 0, ROAD_WIDTH, WINDOW_HEIGHT };
    SDL_RenderFillRect(renderer, &verticalRoad);

    // Draw lane markings
    SDL_SetRenderDrawColor(renderer, 251, 255, 0, 1);

    // Horizontal lane markings
    for (int i = 1; i < 3; i++) {
        int y = (WINDOW_HEIGHT - ROAD_WIDTH) / 1.95  + i * LANE_WIDTH;
        for (int x = 0; x < WINDOW_WIDTH; x += 60) {
            SDL_Rect dash = {x, y - 5, 30, 6};
            if (x < (WINDOW_WIDTH - ROAD_WIDTH) / 1.99 || x > (WINDOW_WIDTH + ROAD_WIDTH) / 2.1) {
                SDL_RenderFillRect(renderer, &dash);
            }
        }
    }

    // Vertical lane markings
    for (int i = 1; i < 3; i++) {
        int x = (WINDOW_WIDTH - ROAD_WIDTH) / 1.98 + i * LANE_WIDTH;
        for (int y = 0; y < WINDOW_HEIGHT; y += 60) {
            SDL_Rect dash = {x - 2, y, 6, 30};
            if (y < (WINDOW_HEIGHT - ROAD_WIDTH) / 2 || y > (WINDOW_HEIGHT + ROAD_WIDTH) / 2.098) {
                SDL_RenderFillRect(renderer, &dash);
            }
        }
    }
}


void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y){
    // display necessary text
    SDL_Color textColor = {0, 0, 0, 255}; // black color
    SDL_Surface *textSurface = TTF_RenderText_Solid(font, text, textColor);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    SDL_Rect textRect = {x,y,0,0 };
    SDL_QueryTexture(texture, NULL, NULL, &textRect.w, &textRect.h);
    SDL_Log("DIM of SDL_Rect %d %d %d %d", textRect.x, textRect.y, textRect.h, textRect.w);
    // SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    // SDL_Log("TTF_Error: %s\n", TTF_GetError());
    SDL_RenderCopy(renderer, texture, NULL, &textRect);
    // SDL_Log("TTF_Error: %s\n", TTF_GetError());
}

void* chequeQueue(void* arg){
    SharedData* sharedData = (SharedData*)arg;
    int i = 1;
    while (1) {
        sharedData->nextLight = 0;
        sleep(5);
        sharedData->nextLight = 2;
        sleep(5);
    }
}

