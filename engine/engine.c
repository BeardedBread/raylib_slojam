#include "engine.h"
#include "mempool.h"

#include "raymath.h"

void init_engine(GameEngine_t* engine)
{
    InitAudioDevice();
    sc_queue_init(&engine->key_buffer);
    engine->sfx_list.n_sfx = N_SFX;
    memset(engine->sfx_list.sfx, 0, engine->sfx_list.n_sfx * sizeof(SFX_t));
    init_memory_pools();
    init_assets(&engine->assets);
}

void deinit_engine(GameEngine_t* engine)
{
    term_assets(&engine->assets);
    free_memory_pools();
    sc_queue_term(&engine->key_buffer);
    CloseAudioDevice();
}

void process_inputs(GameEngine_t* engine, Scene_t* scene)
{
    Vector2 raw_mouse_pos = GetMousePosition();

    if ((scene->state_bits & ACTION_BIT))
    {
        scene->mouse_pos = raw_mouse_pos;
    }
    if (scene->subscene != NULL && (scene->subscene->state_bits & ACTION_BIT))
    {
        scene->subscene->mouse_pos = Vector2Subtract(raw_mouse_pos, scene->subscene_pos);
    }

    unsigned int sz = sc_queue_size(&engine->key_buffer);
    // Process any existing pressed key
    for (size_t i = 0; i < sz; i++)
    {
        int button = sc_queue_del_first(&engine->key_buffer);
        ActionType_t action = sc_map_get_64(&scene->action_map, button);
        if (IsKeyReleased(button))
        {
            do_action(scene, action, false);
        }
        else
        {
            do_action(scene, action, true);
            sc_queue_add_last(&engine->key_buffer, button);
        }
    }

    // Detect new key presses
    while(true)
    {
        int button = GetKeyPressed();
        if (button == 0) break;
        ActionType_t action = sc_map_get_64(&scene->action_map, button);
        if (!sc_map_found(&scene->action_map)) continue;
        do_action(scene, action, true);
        sc_queue_add_last(&engine->key_buffer, button);
    }


    // Mouse button handling
    ActionType_t action = sc_map_get_64(&scene->action_map, MOUSE_BUTTON_RIGHT);
    if (sc_map_found(&scene->action_map))
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
        {
            do_action(scene, action, true);
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT))
        {
            do_action(scene, action, false);
        }
    }
    action = sc_map_get_64(&scene->action_map, MOUSE_BUTTON_LEFT);
    if (sc_map_found(&scene->action_map))
    {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            do_action(scene, action, true);
        }
        else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        {
            do_action(scene, action, false);
        }
    }
}

void change_scene(GameEngine_t* engine, unsigned int idx)
{
    engine->scenes[engine->curr_scene]->state_bits = 0;
    engine->curr_scene = idx;
    engine->scenes[engine->curr_scene]->state_bits = 0b111;
}

bool load_sfx(GameEngine_t* engine, const char* snd_name, uint32_t tag_idx)
{
    if (tag_idx >= engine->sfx_list.n_sfx) return false;
    Sound* snd = get_sound(&engine->assets, snd_name);
    if (snd == NULL) return false;
    engine->sfx_list.sfx[tag_idx].snd = snd;
    engine->sfx_list.sfx[tag_idx].cooldown = 0;
    engine->sfx_list.sfx[tag_idx].plays = 0;
    return true;
}

void set_sfx_pitch(GameEngine_t* engine, uint32_t tag_idx, float pitch)
{
    if (tag_idx >= engine->sfx_list.n_sfx) return;
    SFX_t* sfx = engine->sfx_list.sfx + tag_idx;
    SetSoundPitch(*sfx->snd, pitch);
}

void play_sfx(GameEngine_t* engine, unsigned int tag_idx, bool allow_overlay)
{
    if (tag_idx >= engine->sfx_list.n_sfx) return;
    SFX_t* sfx = engine->sfx_list.sfx + tag_idx;

    if (sfx->snd != NULL)
    {
        if (allow_overlay || (sfx->plays == 0 && !IsSoundPlaying(*sfx->snd)))
        //if (sfx->snd != NULL)
        {
            PlaySound(*sfx->snd);
            sfx->plays++;
        }
    }
}

void stop_sfx(GameEngine_t* engine, unsigned int tag_idx)
{
    if (tag_idx >= engine->sfx_list.n_sfx) return;
    SFX_t* sfx = engine->sfx_list.sfx + tag_idx;
    if (sfx->snd != NULL && IsSoundPlaying(*sfx->snd))
    {
        StopSound(*sfx->snd);
        //sfx->plays--;
    }
}

void update_sfx_list(GameEngine_t* engine)
{
    for (uint32_t i = 0; i< engine->sfx_list.n_sfx; ++i)
    {
        if (!IsSoundPlaying(*engine->sfx_list.sfx->snd))
        {
            engine->sfx_list.sfx[i].plays = 0;
        }
    }
    engine->sfx_list.played_sfx = 0;
}

//void init_scene(Scene_t* scene, SceneType_t scene_type, system_func_t render_func, action_func_t action_func)
void init_scene(Scene_t* scene, render_func_t render_func, action_func_t action_func)
{
    sc_map_init_64(&scene->action_map, 32, 0);
    sc_array_init(&scene->systems);
    init_entity_manager(&scene->ent_manager);
    init_particle_system(&scene->part_sys);

    //scene->scene_type = scene_type;
    scene->render_function = render_func;
    scene->action_function = action_func;
    scene->state_bits = 0b000;
}

void free_scene(Scene_t* scene)
{
    sc_map_term_64(&scene->action_map);
    sc_array_term(&scene->systems);
    free_entity_manager(&scene->ent_manager);
    deinit_particle_system(&scene->part_sys);
}

inline void update_scene(Scene_t* scene, float delta_time)
{
    scene->delta_time = delta_time * scene->time_scale;
    system_func_t sys;
    sc_array_foreach(&scene->systems, sys)
    {
        sys(scene);
    }

    if (scene->type == MAIN_SCENE && scene->subscene != NULL)
    {
        scene->subscene->delta_time = delta_time * scene->subscene->time_scale;
        sc_array_foreach(&scene->subscene->systems, sys)
        {
            sys(scene->subscene);
        }
    }

    update_particle_system(&scene->part_sys, scene->delta_time * scene->time_scale);
}

inline void render_scene(Scene_t* scene)
{
    if (scene->render_function != NULL)
    {
        scene->render_function(scene);
    }
}

inline void do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    ActionResult res = ACTION_PROPAGATE;
    if ((scene->state_bits & ACTION_BIT) && scene->action_function != NULL)
    {
        res = scene->action_function(scene, action, pressed);
    }
    if (scene->subscene != NULL && res == ACTION_PROPAGATE)
    {
        // Recurse for subscene
        do_action(scene->subscene, action, pressed);
    }
}
