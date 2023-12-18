#include "raylib.h"
#include "scenes.h"

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

EntityManager_t ent_manager = {0};

Scene_t* scenes[4];
static GameEngine_t engine = {
    .scenes = scenes,
    .max_scenes = 4,
    .curr_scene = 0,
    .assets = {0}
};

const int screenWidth = 1440;
const int screenHeight = 960;
const float DT = 1/60.0;
const uint8_t MAX_STEPS = 10;

void update_loop(void)
{
    Scene_t* scene = engine.scenes[engine.curr_scene];
    process_inputs(&engine, scene);

    float frame_time = GetFrameTime();
    float delta_time = fminf(frame_time, DT);
    //uint8_t sim_steps = 0;
    //while (frame_time > 1e-5 && sim_steps < MAX_STEPS)
    {
        update_scene(scene, delta_time);
        //frame_time -= delta_time;
        //sim_steps++;
    }

    update_entity_manager(&scene->ent_manager);
    render_scene(scene);
    //update_sfx_list(&engine);
}

int main(void) 
{
    // Initialization
    //--------------------------------------------------------------------------------------
    init_engine(&engine);
    InitWindow(screenWidth, screenHeight, "raylib");

    LevelScene_t lvl_scene;
    ShopScene_t shop_scene;
    scenes[0] = (Scene_t*)&lvl_scene;
    scenes[1] = (Scene_t*)&shop_scene;

    lvl_scene.scene.engine = &engine;
    lvl_scene.scene.subscene = &shop_scene.scene;
    shop_scene.scene.engine = &engine;
    init_level_scene(&lvl_scene);
    change_scene(&engine, 0);

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
    free_level_scene(&lvl_scene);
    deinit_engine(&engine);
    CloseWindow(); 
}
