#include <SDL2/SDL.h>  
#include <stdio.h>  

int main(int argc, char *argv[]) {  
    printf("Initializing SDL...\n");  // Debug statement  
    // Initialize SDL  
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {  
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());  
        return 1;  
    }  

    printf("Creating window...\n");  // Debug statement  
    // Create a window  
    SDL_Window *window = SDL_CreateWindow(  
        "SDL2 Window",                // Window title  
        SDL_WINDOWPOS_UNDEFINED,      // Initial x position  
        SDL_WINDOWPOS_UNDEFINED,      // Initial y position  
        640,                           // Width  
        480,                           // Height  
        SDL_WINDOW_SHOWN              // Window flags  
    );  

    // Check if window was created successfully  
    if (window == NULL) {  
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());  
        SDL_Quit();  
        return 1;  
    }  

    // Main event loop  
    SDL_Event event;  
    int running = 1;  
    printf("Entering the main loop...\n");  // Debug statement  
    while (running) {  
        // Handle events on the queue  
        while (SDL_PollEvent(&event) != 0) {  
            // User requests quit  
            if (event.type == SDL_QUIT) {  
                running = 0;  
            }  
        }  
    }  

    // Destroy the window  
    SDL_DestroyWindow(window);  
    // Quit SDL subsystems  
    SDL_Quit();  

    return 0;  
}