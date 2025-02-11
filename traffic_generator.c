#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define FILENAME "vehicles.data"
#define MIN_VEHICLES 1
#define MAX_VEHICLES 5
#define TOTAL_LANES 4

// Function to generate a random vehicle number
void generateVehicleNumber(char* buffer) {
    buffer[0] = 'A' + rand() % 26;
    buffer[1] = 'A' + rand() % 26;
    buffer[2] = '0' + rand() % 10;
    buffer[3] = 'A' + rand() % 26;
    buffer[4] = 'A' + rand() % 26;
    buffer[5] = '0' + rand() % 10;
    buffer[6] = '0' + rand() % 10;
    buffer[7] = '0' + rand() % 10;
    buffer[8] = '\0';
}

// Function to generate vehicles for a specific lane
void generateRandomVehicles(FILE* file) {
    // Generate a random number of vehicles (1 to MAX_VEHICLES)
    int total_vehicles = (rand() % MAX_VEHICLES) + MIN_VEHICLES;
    
    // Available lanes
    char lanes[] = {'A', 'B', 'C', 'D'};
    
    // Generate vehicles
    for (int i = 0; i < total_vehicles; i++) {
        char vehicle[9];
        generateVehicleNumber(vehicle);
        
        // Randomly select a lane independently for each vehicle
        char chosen_lane = lanes[rand() % TOTAL_LANES];
        
        fprintf(file, "%s:%c\n", vehicle, chosen_lane);
        printf("Generated vehicle: %s for lane %c\n", vehicle, chosen_lane);
    }
}

int main() {
    FILE* file = fopen(FILENAME, "w");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    srand(time(NULL));

    while (1) {
        file = freopen(FILENAME, "w", file);
        if (!file) {
            perror("Error reopening file");
            return 1;
        }
        
        generateRandomVehicles(file);
        fflush(file);
        sleep(2);
    }

    fclose(file);
    return 0;
}