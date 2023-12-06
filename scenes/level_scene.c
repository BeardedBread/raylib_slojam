#include "assets_tag.h"
#include "ent_impl.h"
#include "game_systems.h"
#include "constants.h"

#include <stdio.h>

static void level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    //LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    Entity_t* p_player;
    //sc_map_foreach_value(&scene->ent_manager.component_map[CPLAYERSTATE_T], p_playerstate)
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CPlayerState_t* p_playerstate = get_component(p_player, CPLAYERSTATE_T);
        switch(action)
        {
            case ACTION_UP:
                p_playerstate->player_dir.y = (pressed)? -1 : 0;
            break;
            case ACTION_DOWN:
                p_playerstate->player_dir.y = (pressed)? 1 : 0;
            break;
            case ACTION_LEFT:
                p_playerstate->player_dir.x = (pressed)? -1 : 0;
            break;
            case ACTION_RIGHT:
                p_playerstate->player_dir.x = (pressed)? 1 : 0;
            break;
            case ACTION_BOOST:
                p_playerstate->boosting |= (pressed) ? 1 : 0;
            break;
            default:
            break;
        }
    }
    return;
}

static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    
    Rectangle draw_rec = data->game_rec;
    draw_rec.x = 0;
    draw_rec.y = 0;
    draw_rec.height *= -1;

    BeginDrawing();
        ClearBackground(LIGHTGRAY);
        DrawTextureRec(
            data->game_viewport.texture,
            draw_rec,
            (Vector2){data->game_rec.x, data->game_rec.y},
            WHITE
        );
    EndDrawing();
}

static void arena_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    Entity_t* p_ent;
    static char buffer[32];
    
    BeginTextureMode(data->game_viewport);
        ClearBackground(RAYWHITE);
        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
        {
            snprintf(buffer, 32, "%.3f %.3f", p_ent->position.x, p_ent->position.y);
            DrawCircleV(p_ent->position, p_ent->size, PURPLE);
            DrawText(buffer, 64, 64, 24, RED);
        }
    EndTextureMode();
}

void init_level_scene(LevelScene_t* scene)
{
    init_scene(&scene->scene, &level_scene_render_func, &level_do_action);
    init_entity_tag_map(&scene->scene.ent_manager, PLAYER_ENT_TAG, 4);

    scene->data.game_field_size = (Vector2){ARENA_SIZE, ARENA_SIZE};
    scene->data.game_viewport = LoadRenderTexture(ARENA_SIZE, ARENA_SIZE);
    scene->data.game_rec = (Rectangle){25, 25, ARENA_SIZE, ARENA_SIZE};
    
    create_player(&scene->scene.ent_manager);
    update_entity_manager(&scene->scene.ent_manager);

    
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &global_external_forces_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &player_dir_reset_system);
    sc_array_add(&scene->scene.systems, &arena_render_func);
    
    sc_map_put_64(&scene->scene.action_map, KEY_W, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_S, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_A, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_D, ACTION_RIGHT);
    sc_map_put_64(&scene->scene.action_map, MOUSE_BUTTON_RIGHT, ACTION_BOOST);
}

void free_level_scene(LevelScene_t* scene)
{
    UnloadRenderTexture(scene->data.game_viewport); // Unload render texture
    free_scene(&scene->scene);
}
