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

static int load_all_assets(Assets_t* assets)
{
    Texture2D* game_tex = add_texture(assets, "ast_tex", "res/asteroid_sprites.png");
    if (game_tex == NULL)
    {
        puts("Cannot add game texture! Aborting!");
        return 1;
    }
    Sprite_t* spr;
    char buffer[32];
    for (unsigned int i = 0; i < 3; i++)
    {
        sprintf(buffer, "plr_wep%u", i+1);
        spr = add_sprite(assets, buffer, game_tex);
        spr->origin = (Vector2){32 * i,0};
        spr->frame_size = (Vector2){32,32};
        spr->anchor = (Vector2){16,16};
        spr->frame_count = 1;
        spr->speed = 0;
    }

    for (unsigned int i = 0; i < 3; i++)
    {
        sprintf(buffer, "blt%u", i+1);
        spr = add_sprite(assets, buffer, game_tex);
        spr->origin = (Vector2){96 + 32 * i,0};
        spr->frame_size = (Vector2){32,32};
        spr->anchor = (Vector2){16,16};
        spr->frame_count = 1;
        spr->speed = 0;
    }

    for (unsigned int i = 0; i < 8; i++)
    {
        sprintf(buffer, "upg%u_icon", i+1);
        spr = add_sprite(assets, buffer, game_tex);
        spr->origin = (Vector2){192 + 64 * i,0};
        spr->frame_size = (Vector2){64,64};
        spr->anchor = (Vector2){32,32};
        spr->frame_count = 1;
        spr->speed = 0;
    }

    spr = add_sprite(assets, "enm_normal", game_tex);
    spr->origin = (Vector2){0,32};
    spr->frame_size = (Vector2){128,128};
    spr->anchor = (Vector2){64,64};
    spr->frame_count = 1;
    spr->speed = 0;
    EmitterConfig_t emitter_conf = {
        .launch_range = {-10, 10},
        .speed_range = {200,340},
        .angle_range = {0,360},
        .rotation_range = {-20,20},
        .size_range = {3, 6},
        .particle_lifetime = {0.2,0.3},
        .initial_spawn_delay = 0,
        .type = EMITTER_BURST,
        .one_shot = true,
    };
    
    EmitterConfig_t* conf = add_emitter_conf(assets, "part_hit");
    *conf = emitter_conf;

    return 0;
}

int main(void) 
{
    // Initialization
    //--------------------------------------------------------------------------------------
    init_engine(&engine);
    InitWindow(screenWidth, screenHeight, "raylib");

    if (load_all_assets(&engine.assets) != 0) return 1;

    init_player_creation(&engine.assets);
    init_bullet_creation(&engine.assets);
    init_enemies_creation(&engine.assets);

    static LevelScene_t lvl_scene;
    static ShopScene_t shop_scene;
    scenes[0] = (Scene_t*)&lvl_scene;
    scenes[1] = (Scene_t*)&shop_scene;

    lvl_scene.scene.engine = &engine;
    lvl_scene.scene.subscene = &shop_scene.scene;
    shop_scene.scene.engine = &engine;
    init_level_scene(&lvl_scene);

    Image weapon_img = LoadImage("res/weapon_icons.png");
    lvl_scene.data.weapon_icons = weapon_img;

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
    UnloadImage(weapon_img);
    free_level_scene(&lvl_scene);
    deinit_engine(&engine);
    CloseWindow(); 
}
