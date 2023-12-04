#include "raylib.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>
    EM_BOOL keyDownCallback(int eventType, const EmscriptenKeyboardEvent *event, void* userData) {
        return true; // Just preventDefault everything lol
    }
#endif

const int screenWidth = 1280;
const int screenHeight = 640;
const float DT = 1/60.0;
const uint8_t MAX_STEPS = 10;

void update_function(float delta_time)
{
    char buf[32];
    sprintf(buf, "%.6f\n", delta_time);
}

void update_loop(void)
{
    static char buffer[32];

    float frame_time = GetFrameTime();
    snprintf(buffer, 32, "Ready to go: %.5f",frame_time);
    uint8_t sim_steps = 0;

    while (frame_time > 1e-5 && sim_steps < MAX_STEPS)
    {
        float delta_time = fminf(frame_time, DT);
        update_function(delta_time);
        frame_time -= delta_time;
        sim_steps++;
    }
    //update_function(DT);
    BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText(buffer, 64, 64, 24, RED);
    EndDrawing();
}


int main(void) 
{
    // Initialization
    //--------------------------------------------------------------------------------------
    //InitAudioDevice();
    InitWindow(screenWidth, screenHeight, "raylib");
    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    #if defined(PLATFORM_WEB)
        puts("Setting emscripten main loop");
        emscripten_set_keypress_callback("#canvas", NULL, 1, keyDownCallback);
        emscripten_set_keydown_callback("#canvas", NULL, 1, keyDownCallback);
        emscripten_set_main_loop(update_loop, 0, 1);
    #else
    while (!WindowShouldClose())
    {
        update_loop();
    }
    #endif
    CloseWindow(); 
    //CloseAudioDevice();
}
