#include "assets_tag.h"
#include "ent_impl.h"
#include "game_systems.h"

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
            default:
            break;
        }
    }
    return;
}

static void level_scene_render_func(Scene_t* scene)
{
    static char buffer[32];
    BeginDrawing();
        ClearBackground(RAYWHITE);
        
        Entity_t* p_ent;
        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
        {
            snprintf(buffer, 32, "%.3f %.3f", p_ent->position.x, p_ent->position.y);
            DrawCircleV(p_ent->position, p_ent->size, PURPLE);
            DrawText(buffer, 64, 64, 24, RED);
        }
    EndDrawing();
}

void init_level_scene(LevelScene_t* scene)
{
    init_scene(&scene->scene, &level_scene_render_func, &level_do_action);
    init_entity_tag_map(&scene->scene.ent_manager, PLAYER_ENT_TAG, 4);

    scene->data.game_field_size = (Vector2){640,640};
    
    create_player(&scene->scene.ent_manager);
    update_entity_manager(&scene->scene.ent_manager);

    
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &player_dir_reset_system);
    
    sc_map_put_64(&scene->scene.action_map, KEY_W, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_S, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_A, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_D, ACTION_RIGHT);
}

void free_level_scene(LevelScene_t* scene)
{
    free_scene(&scene->scene);
}
