#include "assets_tag.h"
#include "level_ent.h"
#include "game_systems.h"
#include "constants.h"
#include "raymath.h"
#include "mempool.h"
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
            case ACTION_SHOOT:
                p_playerstate->shoot |= (pressed) ? 1 : 0;
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

    static char mem_stats[512];
    print_mempool_stats(mem_stats);
    BeginDrawing();
        ClearBackground(LIGHTGRAY);
        DrawTextureRec(
            data->game_viewport.texture,
            draw_rec,
            (Vector2){data->game_rec.x, data->game_rec.y},
            WHITE
        );
        DrawText(mem_stats, data->game_rec.x + data->game_rec.width, data->game_rec.y, 12, BLACK);

    EndDrawing();
}

static void arena_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    Entity_t* p_ent;
    static char buffer[32];
    
    BeginTextureMode(data->game_viewport);
        ClearBackground(RAYWHITE);
        

        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            if (p_ent->m_tag == PLAYER_ENT_TAG)
            {
                snprintf(buffer, 32, "%.3f %.3f", p_ent->position.x, p_ent->position.y);
                DrawCircleV(p_ent->position, p_ent->size, PURPLE);
                
                int8_t flip_x = 0;
                int8_t flip_y = 0;
                if (p_ent->position.x < p_ent->size)
                {
                    flip_x = 1;
                }
                if (p_ent->position.x > data->game_field_size.x - p_ent->size)
                {
                    flip_x = -1;
                }

                if (p_ent->position.y < p_ent->size)
                {
                    flip_y = 1;
                }
                if (p_ent->position.y > data->game_field_size.y - p_ent->size)
                {
                    flip_y = -1;
                }

                Vector2 clone_pos = p_ent->position;
                if (flip_x != 0 && flip_y != 0)
                {
                    clone_pos.x += flip_x * data->game_field_size.x;
                    DrawCircleV(clone_pos, p_ent->size, PURPLE);
                    clone_pos.y += flip_y * data->game_field_size.y;
                    DrawCircleV(clone_pos, p_ent->size, PURPLE);
                    clone_pos.x -= flip_x * data->game_field_size.x;
                    DrawCircleV(clone_pos, p_ent->size, PURPLE);
                }
                else if (flip_x != 0)
                {
                    clone_pos.x += flip_x * data->game_field_size.x;
                    DrawCircleV(clone_pos, p_ent->size, PURPLE);
                }
                else
                {
                    clone_pos.y += flip_y * data->game_field_size.y;
                    DrawCircleV(clone_pos, p_ent->size, PURPLE);
                }

                CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
                Vector2 look_dir = Vector2Add(
                    p_ent->position,
                    Vector2Scale(p_pstate->aim_dir, 64)
                );
                DrawCircleV(look_dir, 8, BLACK);
                DrawText(buffer, 64, 64, 24, RED);
            }
            else if (p_ent->m_tag == ENEMY_ENT_TAG)
            {
                DrawCircleV(p_ent->position, p_ent->size, RED);
            }
            else
            {
                DrawCircleV(p_ent->position, p_ent->size, BLUE);
            }
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
    create_spawner(&scene->scene.ent_manager);
    update_entity_manager(&scene->scene.ent_manager);

    
    sc_array_add(&scene->scene.systems, &weapon_cooldown_system);
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &global_external_forces_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &hitbox_update_system);
    sc_array_add(&scene->scene.systems, &player_dir_reset_system);
    sc_array_add(&scene->scene.systems, &life_update_system);
    sc_array_add(&scene->scene.systems, &ai_update_system);
    sc_array_add(&scene->scene.systems, &spawned_update_system);
    sc_array_add(&scene->scene.systems, &arena_render_func);
    
    sc_map_put_64(&scene->scene.action_map, KEY_W, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_S, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_A, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_D, ACTION_RIGHT);
    sc_map_put_64(&scene->scene.action_map, MOUSE_BUTTON_RIGHT, ACTION_BOOST);
    sc_map_put_64(&scene->scene.action_map, MOUSE_BUTTON_LEFT, ACTION_SHOOT);
}

void free_level_scene(LevelScene_t* scene)
{
    UnloadRenderTexture(scene->data.game_viewport); // Unload render texture
    free_scene(&scene->scene);
}
