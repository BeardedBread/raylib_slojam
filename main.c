#include "raylib.h"
#include "scenes.h"
#include "assets_tag.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>
    EM_BOOL keyDownCallback(int eventType, const EmscriptenKeyboardEvent *event, void* userData) {
        return true; // Just preventDefault everything lol
    }
    EM_BOOL mouseClickCallback(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData)
    {
        // Disable cursor centers the cursor, just ignore that
        //DisableCursor();
        emscripten_request_pointerlock("#canvas", 1);
        return true;
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
const int screenHeight = 900;
const float DT = 1/60.0;
const uint8_t MAX_STEPS = 10;

static Music ambient_mus;
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
    update_sfx_list(&engine);

    UpdateMusicStream(ambient_mus);
    if (!IsMusicStreamPlaying(ambient_mus))
    {
        PlayMusicStream(ambient_mus);
    }
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

    spr = add_sprite(assets, "finale1", game_tex);
    spr->origin = (Vector2){128,32};
    spr->frame_size = (Vector2){64,64};
    spr->anchor = (Vector2){32,32};
    spr->frame_count = 1;
    spr->speed = 0;

    spr = add_sprite(assets, "finale2", game_tex);
    spr->origin = (Vector2){128,96};
    spr->frame_size = (Vector2){64,64};
    spr->anchor = (Vector2){32,32};
    spr->frame_count = 1;
    spr->speed = 0;

    spr = add_sprite(assets, "ms_ctrl", game_tex);
    spr->origin = (Vector2){256,64};
    spr->frame_size = (Vector2){192,256};
    spr->anchor = (Vector2){96,128};
    spr->frame_count = 1;
    spr->speed = 0;

    spr = add_sprite(assets, "kb_ctrl", game_tex);
    spr->origin = (Vector2){448,64};
    spr->frame_size = (Vector2){352,256};
    spr->anchor = (Vector2){176,128};
    spr->frame_count = 1;
    spr->speed = 0;

    spr = add_sprite(assets, "crosshair", game_tex);
    spr->origin = (Vector2){0,160};
    spr->frame_size = (Vector2){32,32};
    spr->anchor = (Vector2){16,16};
    spr->frame_count = 1;
    spr->speed = 0;

    spr = add_sprite(assets, "finale3", game_tex);
    spr->origin = (Vector2){192,96};
    spr->frame_size = (Vector2){64,64};
    spr->anchor = (Vector2){32,32};
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

    emitter_conf = (EmitterConfig_t){
        .launch_range = {-180, 180},
        .speed_range = {400,450},
        .angle_range = {0,360},
        .rotation_range = {-20,20},
        .size_range = {2, 5},
        .particle_lifetime = {0.1,0.2},
        .initial_spawn_delay = 0,
        .type = EMITTER_BURST,
        .one_shot = true,
    };
    
    conf = add_emitter_conf(assets, "part_ded");
    *conf = emitter_conf;

    emitter_conf = (EmitterConfig_t){
        .launch_range = {-30, 30},
        .speed_range = {550,750},
        .angle_range = {0,1},
        .rotation_range = {0,1},
        .size_range = {3, 5},
        .particle_lifetime = {0.1f,0.2f},
        .initial_spawn_delay = 0.05f,
        .type = EMITTER_STREAM,
        .one_shot = true,
    };
    
    conf = add_emitter_conf(assets, "part_fire");
    *conf = emitter_conf;

    add_sound(assets, "snd_fly", "res/fly.ogg");
    load_sfx(&engine, "snd_fly", PLAYER_MOVE_SFX);
    add_sound(assets, "snd_boost", "res/boost.ogg");
    load_sfx(&engine, "snd_boost", PLAYER_BOOST_SFX);
    add_sound(assets, "snd_nails", "res/nails.ogg");
    load_sfx(&engine, "snd_nails", WEAPON1_FIRE_SFX);
    add_sound(assets, "snd_thump", "res/thumper.ogg");
    load_sfx(&engine, "snd_thump", WEAPON2_FIRE_SFX);
    add_sound(assets, "snd_maws", "res/maws.ogg");
    load_sfx(&engine, "snd_maws", WEAPON3_FIRE_SFX);
    
    add_sound(assets, "snd_nailsh", "res/nail_hit.ogg");
    load_sfx(&engine, "snd_nailsh", WEAPON1_HIT_SFX);
    add_sound(assets, "snd_thumph", "res/thumper_hit.ogg");
    load_sfx(&engine, "snd_thumph", WEAPON2_HIT_SFX);
    add_sound(assets, "snd_mawsh", "res/maw_hit.ogg");
    load_sfx(&engine, "snd_mawsh", WEAPON3_HIT_SFX);

    add_sound(assets, "snd_enmded", "res/enemy_des.ogg");
    load_sfx(&engine, "snd_enmded", ENEMY_DEAD_SFX);

    add_sound(assets, "snd_coin", "res/coin.ogg");
    load_sfx(&engine, "snd_coin", COLLECT_SFX);

    add_sound(assets, "snd_buy", "res/buy.ogg");
    load_sfx(&engine, "snd_buy", BUYING_SFX);

    add_sound(assets, "snd_no", "res/nope.ogg");
    load_sfx(&engine, "snd_no", NOPE_SFX);

    add_sound(assets, "snd_warn", "res/warn.ogg");
    load_sfx(&engine, "snd_warn", PLAYER_WARN_SFX);

    add_sound(assets, "snd_ready", "res/ready.ogg");
    load_sfx(&engine, "snd_ready", PLAYER_READY_SFX);

    add_sound(assets, "snd_buy", "res/death.ogg");
    load_sfx(&engine, "snd_buy", PLAYER_DEATH_SFX);

    add_sound(assets, "snd_pause", "res/pause.ogg");
    load_sfx(&engine, "snd_pause", PAUSE_SFX);

    add_sound(assets, "snd_rnkup", "res/rank2.ogg");
    load_sfx(&engine, "snd_rnkup", RANKUP_SFX);

    add_sound(assets, "snd_end", "res/end.ogg");
    load_sfx(&engine, "snd_end", ENDING_SFX);
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
    init_collectible_creation(&engine.assets);

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

#if !defined(PLATFORM_WEB)
    //DisableCursor();                    // Limit cursor to relative movement inside the window
#endif

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    ambient_mus = LoadMusicStream("res/ambient.ogg");

    #if defined(PLATFORM_WEB)
        puts("Setting emscripten main loop");
        emscripten_set_keypress_callback("#canvas", NULL, 1, keyDownCallback);
        emscripten_set_keydown_callback("#canvas", NULL, 1, keyDownCallback);
        emscripten_set_click_callback("#canvas", NULL, 1, mouseClickCallback);
        emscripten_set_main_loop(update_loop, 0, 1);
    #else
    while (!WindowShouldClose())
    {
        update_loop();
    }
    #endif
    StopMusicStream(ambient_mus);
    UnloadMusicStream(ambient_mus);
    UnloadImage(weapon_img);
    free_level_scene(&lvl_scene);
    deinit_engine(&engine);
    CloseWindow(); 
}
