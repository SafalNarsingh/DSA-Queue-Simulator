#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 1280
#define MAIN_FONT "DejaVuSans.ttf"
#define WINDOW_HEIGHT 720
#define SCALE 1
#define ROAD_WIDTH 160  // Increased to accommodate 3 lanes
#define LANE_WIDTH 30   // Width of each individual lane
#define VEHICLE_SIZE 40
#define VEHICLE_LENGTH 60  // New constant for vehicle length
#define MAX_VEHICLES 200   // Increased to accommodate more vehicles
   // Increased to accommodate more vehicles
#define VEHICLE_SPEED 2

typedef struct {
    char id[9];         // Vehicle ID
    int x, y;          // Position
    char lane;         // A, B, C, or D
    int sublane;       // 1, 2, or 3
    bool active;
    int direction;     // 1 for right/down, -1 for left/up
} Vehicle;

Vehicle vehicles[MAX_VEHICLES];
SDL_mutex* vehicleMutex;


typedef struct {
    bool green; // true if green, false if red
} TrafficLight;

TrafficLight trafficLights[4]; // One for each lane

void initTrafficLights() {
    trafficLights[0].green = true;  // A starts green
    trafficLights[1].green = true; // B starts red
    trafficLights[2].green = true; // C starts red
    trafficLights[3].green = true; // D starts red
}

// Convert lane letter and number to screen coordinates
// Modified getLanePosition function for 2 lanes
void getLanePosition(char lane, int sublane, int* x, int* y) {
    int center_x = WINDOW_WIDTH / 2;
    int center_y = WINDOW_HEIGHT / 2;
    
    switch(lane) {
        case 'A': // Left to Right
            *x = 0;
            *y = center_y - ROAD_WIDTH/3 + (sublane - 1) * LANE_WIDTH;
            break;
        case 'B': // Right to Left
            *x = WINDOW_WIDTH;
            *y = center_y + ROAD_WIDTH/3 - (sublane - 1) * LANE_WIDTH;
            break;
        case 'C': // Top to Bottom
            *x = center_x - ROAD_WIDTH/3 + (sublane - 1) * LANE_WIDTH;
            *y = 0;
            break;
        case 'D': // Bottom to Top
            *x = center_x + ROAD_WIDTH/3 - (sublane - 1) * LANE_WIDTH;
            *y = WINDOW_HEIGHT;
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

void* updateTrafficLights(void* arg) {
    srand(time(NULL)); // Seed the random number generator

    while (1) {
        SDL_LockMutex(vehicleMutex);

        int randomLane = rand() % 4;  // Choose a random lane (0-3)
        int randomSleep = (rand() % 10) + 1; // Random sleep time between 1 and 10 seconds

        // Set all lights to red first
        for (int i = 0; i < 4; i++) {
            trafficLights[i].green = false;
        }

        // Set one random lane to green
        trafficLights[randomLane].green = true;

        SDL_UnlockMutex(vehicleMutex);

        sleep(randomSleep); // Sleep for a random time between 1 and 10 seconds
    }
    return NULL;
}


void spawnVehicle(const char* id, char lane) {
    SDL_LockMutex(vehicleMutex);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (!vehicles[i].active) {
            vehicles[i].active = true;
            strncpy(vehicles[i].id, id, 8);
            vehicles[i].id[8] = '\0';
            vehicles[i].lane = lane;
            vehicles[i].sublane = (rand() % 1) + 1;  // Random sublane (1 or 2)
            vehicles[i].direction = getDirection(lane);
            
            // Set initial position
            getLanePosition(lane, vehicles[i].sublane, &vehicles[i].x, &vehicles[i].y);
            break;
        }
    }
    SDL_UnlockMutex(vehicleMutex);
}

void updateVehicles() {
    SDL_LockMutex(vehicleMutex);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].active) {
            int laneIndex = (vehicles[i].lane == 'A') ? 0 :
                            (vehicles[i].lane == 'B') ? 1 :
                            (vehicles[i].lane == 'C') ? 2 : 3;

            if (!trafficLights[laneIndex].green) continue; // Stop vehicles if the light is red

            switch(vehicles[i].lane) {
                case 'A':
                    vehicles[i].x += VEHICLE_SPEED * vehicles[i].direction;
                        if ((vehicles[i].lane == 'A' && vehicles[i].x > WINDOW_WIDTH + VEHICLE_LENGTH/2) || 
                            (vehicles[i].lane == 'B' && vehicles[i].x < -VEHICLE_LENGTH/2)) {
                            vehicles[i].active = false;
                        }
                        break;
                case 'B':
                    vehicles[i].x += VEHICLE_SPEED * vehicles[i].direction;
                    if ((vehicles[i].lane == 'A' && vehicles[i].x > WINDOW_WIDTH + VEHICLE_LENGTH/2) || 
                        (vehicles[i].lane == 'B' && vehicles[i].x < -VEHICLE_LENGTH/2)) {
                        vehicles[i].active = false;
                    }
                    break;
                case 'C':
                    vehicles[i].y += VEHICLE_SPEED * vehicles[i].direction;
                        if ((vehicles[i].lane == 'C' && vehicles[i].y > WINDOW_HEIGHT + VEHICLE_LENGTH/2) || 
                            (vehicles[i].lane == 'D' && vehicles[i].y < -VEHICLE_LENGTH/2)) {
                            vehicles[i].active = false;
                        }
                        break;
                case 'D':
                    vehicles[i].y += VEHICLE_SPEED * vehicles[i].direction;
                    if ((vehicles[i].lane == 'C' && vehicles[i].y > WINDOW_HEIGHT + VEHICLE_LENGTH/2) || 
                        (vehicles[i].lane == 'D' && vehicles[i].y < -VEHICLE_LENGTH/2)) {
                        vehicles[i].active = false;
                    }
                    break;
            }
        }
    }
    SDL_UnlockMutex(vehicleMutex);
}


void drawTrafficLights(SDL_Renderer* renderer) {
    int center_x = WINDOW_WIDTH / 2;
    int center_y = WINDOW_HEIGHT / 2;
    
    SDL_Rect lights[4] = {
        { center_x - ROAD_WIDTH / 2 - 30, center_y - 60, 20, 40 }, // A ->
        { center_x + ROAD_WIDTH / 2 + 20, center_y + 25, 20, 40 }, // B |
        { center_x - 60, center_y - ROAD_WIDTH / 2 - 40, 40, 20 }, // C ^
        { center_x + 25, center_y + ROAD_WIDTH / 2 + 10, 40, 20 }  // D <-
    };
    
    for (int i = 0; i < 4; i++) {
        if (trafficLights[i].green)
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green
        else
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red

        SDL_RenderFillRect(renderer, &lights[i]);
    }
}

// Update the drawVehicles function in simulator.c:
void drawVehicles(SDL_Renderer* renderer) {
    SDL_LockMutex(vehicleMutex);
    for (int i = 0; i < MAX_VEHICLES; i++) {
        if (vehicles[i].active) {
            // Different colors for different lanes
            switch(vehicles[i].lane) {
                case 'A':
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);   // Red
                    break;
                case 'B':
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);   // Green
                    break;
                case 'C':
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);   // Blue
                    break;
                case 'D':
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // Yellow
                    break;
            }
            
            SDL_Rect rect;
            if (vehicles[i].lane == 'A') {
                // Horizontal vehicles
                rect = (SDL_Rect){ 
                    vehicles[i].x - VEHICLE_LENGTH/2 , 
                    vehicles[i].y - VEHICLE_SIZE/2 + 10, 
                    VEHICLE_LENGTH, 
                    VEHICLE_SIZE 
                };
            }
            else if (vehicles[i].lane == 'B') {
                // Horizontal vehicles
                rect = (SDL_Rect){ 
                    vehicles[i].x - VEHICLE_LENGTH/2 , 
                    vehicles[i].y - VEHICLE_SIZE/2 - 10, 
                    VEHICLE_LENGTH, 
                    VEHICLE_SIZE 
                };
            }
            else if (vehicles[i].lane == 'C') {
                // Vertical vehicles
                rect = (SDL_Rect){ 
                    vehicles[i].x - VEHICLE_LENGTH/2 + 20 , 
                    vehicles[i].y - VEHICLE_SIZE/2 , 
                    VEHICLE_SIZE, 
                    VEHICLE_LENGTH 
                };
            }
            else {
                // Vertical vehicles
                rect = (SDL_Rect){ 
                    vehicles[i].x - VEHICLE_LENGTH/2, 
                    vehicles[i].y - VEHICLE_SIZE/2, 
                    VEHICLE_SIZE, 
                    VEHICLE_LENGTH 
                };
            }
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    SDL_UnlockMutex(vehicleMutex);
}




void* readAndParseFile(void* arg) {
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

            if (vehicleNumber && lane) {
                spawnVehicle(vehicleNumber, lane[0]);
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

typedef struct{
    int currentLight;
    int nextLight;
} SharedData;


// Function declarations
bool initializeSDL(SDL_Window **window, SDL_Renderer **renderer);
void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font);
void displayText(SDL_Renderer *renderer, TTF_Font *font, char *text, int x, int y);
void drawLightForB(SDL_Renderer* renderer, bool isRed);
void refreshLight(SDL_Renderer *renderer, SharedData* sharedData);
void* chequeQueue(void* arg);
void* readAndParseFile(void* arg);
void* mainLoop(void* arg);


void printMessageHelper(const char* message, int count) {
    for (int i = 0; i < count; i++) printf("%s\n", message);
}

int main(int argc, char *argv[]) {
    pthread_t tQueue, tReadFile;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;    
    SDL_Event event;    

    if (!initializeSDL(&window, &renderer)) {
        return -1;
    }
    SDL_mutex* mutex = SDL_CreateMutex();
    SharedData sharedData = { 0, 0 }; // 0 => all red
    vehicleMutex = SDL_CreateMutex();
    initVehicles();
    
    TTF_Font* font = TTF_OpenFont(MAIN_FONT, 24);
    if (!font) SDL_Log("Failed to load font: %s", TTF_GetError());

    updateVehicles();
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    drawRoadsAndLane(renderer, NULL);
    drawVehicles(renderer);
    SDL_RenderPresent(renderer);
    SDL_Delay(16); // Cap at roughly 60 FPS

    // we need to create seprate long running thread for the queue processing and light
    // pthread_create(&tLight, NULL, refreshLight, &sharedData);
    pthread_t renderThread;
    pthread_create(&renderThread, NULL, mainLoop, renderer);
    pthread_create(&tReadFile, NULL, readAndParseFile, NULL);
    pthread_t trafficLightThread;
    pthread_create(&trafficLightThread, NULL, updateTrafficLights, NULL);


    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
        }
        refreshLight(renderer, &sharedData);
        SDL_Delay(16);
    }
    SDL_DestroyMutex(mutex);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    // pthread_kil
    SDL_Quit();
    return 0;
}

// Modified main loop
void* mainLoop(void* arg) {
    SDL_Renderer* renderer = (SDL_Renderer*)arg;
    while (1) {
        updateVehicles();
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        drawRoadsAndLane(renderer, NULL);
        drawTrafficLights(renderer); // Add this to render traffic lights
        drawVehicles(renderer);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
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


void drawLightForB(SDL_Renderer* renderer, bool isRed){
    // draw light box
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_Rect lightBox = {400, 300, 50, 30};
    SDL_RenderFillRect(renderer, &lightBox);
    // draw light
    if(isRed) SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // red
    else SDL_SetRenderDrawColor(renderer, 11, 156, 50, 255);    // green
    SDL_Rect straight_Light = {405, 305, 20, 20};
    SDL_RenderFillRect(renderer, &straight_Light);
    drawArrwow(renderer, 435,305, 435, 305+20, 435+10, 305+10);
}


void drawRoadsAndLane(SDL_Renderer *renderer, TTF_Font *font) {
    SDL_SetRenderDrawColor(renderer, WHITE.r, WHITE.g, WHITE.b, WHITE.a);
    SDL_RenderClear(renderer);

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
    SDL_SetRenderDrawColor(renderer, YELLOW.r, YELLOW.g, YELLOW.b, YELLOW.a);

    // Horizontal lane markings
    for (int i = 1; i < 2; i++) {
        int y = (WINDOW_HEIGHT - ROAD_WIDTH) / 1.7  + i * LANE_WIDTH;
        for (int x = 0; x < WINDOW_WIDTH; x += 60) {
            SDL_Rect dash = {x, y - 2, 30, 6};
            if (x < (WINDOW_WIDTH - ROAD_WIDTH) / 1.99 || x > (WINDOW_WIDTH + ROAD_WIDTH) / 2.1) {
                SDL_RenderFillRect(renderer, &dash);
            }
        }
    }

    // Vertical lane markings
    for (int i = 1; i < 2; i++) {
        int x = (WINDOW_WIDTH - ROAD_WIDTH) / 1.83 + i * LANE_WIDTH;
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


void refreshLight(SDL_Renderer *renderer, SharedData* sharedData){
    if(sharedData->nextLight == sharedData->currentLight) return; // early return

    if(sharedData->nextLight == 0){ // trun off all lights
        drawLightForB(renderer, false);
    }
    if(sharedData->nextLight == 2) drawLightForB(renderer, true);
    else drawLightForB(renderer, false);
    SDL_RenderPresent(renderer);
    printf("Light of queue updated from %d to %d\n", sharedData->currentLight,  sharedData->nextLight);
    // update the light
    sharedData->currentLight = sharedData->nextLight;
    fflush(stdout);
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

// you may need to pass the queue on this function for sharing the data
