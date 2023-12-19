#include "actions.h"
#include "assets_tag.h"
#include "engine.h"
#include "level_ent.h"
#include "game_systems.h"
#include "constants.h"
#include "raymath.h"
#include "mempool.h"
#include <stdio.h>

static ActionResult level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    //LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    Entity_t* p_player;
    //sc_map_foreach_value(&scene->ent_manager.component_map[CPLAYERSTATE_T], p_playerstate)
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CPlayerState_t* p_playerstate = get_component(p_player, CPLAYERSTATE_T);
        CWeaponStore_t* p_weaponstore = get_component(p_player, CWEAPONSTORE_T);
        CWeapon_t* p_weapon = get_component(p_player, CWEAPON_T);
        uint8_t new_weapon = p_weapon->weapon_idx;
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
            case ACTION_SELECT1:
                new_weapon = 0;
            break;
            case ACTION_SELECT2:
                new_weapon = 1;
            break;
            case ACTION_SELECT3:
                new_weapon = 2;
            break;
            case ACTION_SELECT4:
                new_weapon = 3;
            break;
            case ACTION_PAUSE:
                if (!pressed)
                {
                    scene->time_scale = 0.0f;
                    scene->state_bits = 0b110;
                    scene->subscene->state_bits = 0b111;
                    return ACTION_CONSUMED;
                }
            break;
            default:
            break;
        }
        if (new_weapon != p_weapon->weapon_idx)
        {
            if (new_weapon < p_weaponstore->n_weapons && p_weaponstore->unlocked[new_weapon])
            {
                p_weaponstore->weapons[p_weapon->weapon_idx] = *p_weapon;
                *p_weapon = p_weaponstore->weapons[new_weapon];
            }
        }
    }
    return ACTION_PROPAGATE;
}

static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    ShopScene_t* shop_scene = (ShopScene_t*)scene->subscene;
    
    Rectangle draw_rec = data->game_rec;
    draw_rec.x = 0;
    draw_rec.y = 0;
    draw_rec.height *= -1;


    Entity_t* p_ent;
    BeginDrawing();
        ClearBackground(LIGHTGRAY);
        DrawTextureRec(
            data->game_viewport.texture,
            draw_rec,
            (Vector2){data->game_rec.x, data->game_rec.y},
            WHITE
        );

        draw_rec = shop_scene->data.shop_rec;
        draw_rec.x = 0;
        draw_rec.y = 0;
        draw_rec.height *= -1;
        DrawTextureRec(
            shop_scene->data.shop_viewport.texture,
            draw_rec,
            (Vector2){
                shop_scene->data.shop_rec.x + scene->subscene_pos.x,
                shop_scene->data.shop_rec.y = scene->subscene_pos.y
            },
            WHITE
        );

        static char mem_stats[512];
        print_mempool_stats(mem_stats);
        DrawText(mem_stats, data->game_rec.x + 10, data->game_rec.y, 12, BLACK);
        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
        {
            CLifeTimer_t* p_life = get_component(p_ent, CLIFETIMER_T);
            sprintf(mem_stats, "HP: %u/%u ( %.2f )", p_life->current_life, p_life->max_life, p_life->corruption);
            DrawText(
                mem_stats, scene->subscene_pos.x,
                scene->subscene_pos.y + shop_scene->data.shop_rec.height + 10,
                32,BLACK
            );

            CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
            sprintf(mem_stats, "Redirection: %.2f", p_pstate->boost_cooldown);
            DrawText(
                mem_stats, scene->subscene_pos.x,
                scene->subscene_pos.y + shop_scene->data.shop_rec.height + 42,
                32,BLACK
            );

            CWeapon_t* p_weapon = get_component(p_ent, CWEAPON_T);
            const char* weapon_name = "?";
            switch (p_weapon->weapon_idx)
            {
                case 0: weapon_name = "Nails";break;
                case 1: weapon_name = "Thumper";break;
                case 2: weapon_name = "Maws";break;
                default: break;
            }
            sprintf(mem_stats, "%s: %.2f", weapon_name, p_weapon->cooldown_timer);
            DrawText(
                mem_stats, scene->subscene_pos.x,
                scene->subscene_pos.y + shop_scene->data.shop_rec.height + 74,
                32,BLACK
            );
            break;
        }

    EndDrawing();
}

static void arena_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    Entity_t* p_ent;
    
    BeginTextureMode(data->game_viewport);
        ClearBackground(RAYWHITE);
        BeginMode2D(data->cam);

        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            Color c = BLUE;
            if (p_ent->m_tag == ENEMY_ENT_TAG)
            {
                c = RED;
            }
            else if (p_ent->m_tag == PLAYER_ENT_TAG)
            {
                c = PURPLE;
            }
            DrawCircleV(p_ent->position, p_ent->size, c);

            CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
            if (p_ct != NULL && p_ct->edge_b == EDGE_WRAPAROUND)
            {
                
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
                    DrawCircleV(clone_pos, p_ent->size, c);
                    clone_pos.y += flip_y * data->game_field_size.y;
                    DrawCircleV(clone_pos, p_ent->size, c);
                    clone_pos.x -= flip_x * data->game_field_size.x;
                    DrawCircleV(clone_pos, p_ent->size, c);
                }
                else if (flip_x != 0)
                {
                    clone_pos.x += flip_x * data->game_field_size.x;
                    DrawCircleV(clone_pos, p_ent->size, c);
                }
                else
                {
                    clone_pos.y += flip_y * data->game_field_size.y;
                    DrawCircleV(clone_pos, p_ent->size, c);
                }

            }
            
            CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
            if (p_pstate != NULL)
            {
                Vector2 look_dir = Vector2Add(
                    p_ent->position,
                    Vector2Scale(p_pstate->aim_dir, 64)
                );
                DrawLineEx(p_ent->position, look_dir, 2, BLACK);
            }
        }
        Vector2 raw_mouse_pos = GetMousePosition();
        raw_mouse_pos = Vector2Subtract(raw_mouse_pos, (Vector2){data->game_rec.x, data->game_rec.y});
        DrawCircleV(raw_mouse_pos, 2, BLACK);

        EndMode2D();

    EndTextureMode();
}

void shop_render_func(Scene_t* scene)
{
    ShopSceneData* data = &(((ShopScene_t*)scene)->data);
    //Entity_t* p_ent;
    const int ICON_SIZE = 75; 
    const int ICON_HALF_SIZE = (ICON_SIZE >> 1); 
    const int DOT_SIZE = 10;
    const int DOT_SPACING = 60;
    const int DESCRPTION_BOX_WIDTH = 330;
    const int DESCRPTION_BOX_HEIGHT = 130;
    const int start_x = 50;
    const int start_y = 50;
    BeginTextureMode(data->shop_viewport);
        ClearBackground(RAYWHITE);
        if (scene->state_bits & RENDER_BIT)
        {
            Vector2 center = {start_x, start_y};
            // Stats Upgrades: Hardcoded to 4 upgrades lol
            for (uint8_t i = 0; i < 4; ++i)
            {
                Vector2 tl = {center.x - ICON_HALF_SIZE, center.y - ICON_HALF_SIZE};
                DrawRectangleV(tl, (Vector2){ICON_SIZE, ICON_SIZE}, BLACK);

                center.x += ICON_HALF_SIZE + 30;
                for (uint8_t i = 0; i < 3; ++i)
                {
                    DrawCircleV(center, DOT_SIZE, BLACK);
                    center.x += DOT_SPACING;
                }
                center.x -= DOT_SPACING;

                //center.x += BTN_HALF_SIZE;
                //tl = (Vector2){center.x - BTN_HALF_SIZE, center.y - BTN_HALF_SIZE};
                //DrawRectangleV(tl, (Vector2){BTN_SIZE, BTN_SIZE}, BLACK);

                center.x += ICON_HALF_SIZE + 10;
                DrawText("0000", center.x, center.y - (36 >> 1), 36, BLACK);

                center.x = start_x;
                center.y += 100;
            }

            // Full heal
            Vector2 tl = {center.x - ICON_HALF_SIZE, center.y - ICON_HALF_SIZE};
            DrawRectangleV(tl, (Vector2){ICON_SIZE, ICON_SIZE}, BLACK);
            center.x += ICON_HALF_SIZE + 10;
            DrawText("0000", center.x, center.y - (36 >> 1), 36, BLACK);
            center.x += MeasureText("0000", 36) + ICON_HALF_SIZE + 25;

            // Escape
            tl = (Vector2){center.x - ICON_HALF_SIZE, center.y - ICON_HALF_SIZE};
            DrawRectangleV(tl, (Vector2){ICON_SIZE, ICON_SIZE}, BLACK);
            center.x += ICON_HALF_SIZE + 10;
            DrawText("0000", center.x, center.y - (36 >> 1), 36, BLACK);

            center.x = start_x - 20;
            center.y += 20 + ICON_HALF_SIZE;

            DrawRectangleLinesEx(
                (Rectangle){center.x, center.y, DESCRPTION_BOX_WIDTH, DESCRPTION_BOX_HEIGHT},
                3, BLACK
            );
        }

    EndTextureMode();
}

static ActionResult shop_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    switch(action)
    {
        case ACTION_SHOOT:
            if (!pressed)
            {
                printf("Clicked %.2f, %.2f\n", scene->mouse_pos.x, scene->mouse_pos.y);
            }
        break;
        case ACTION_PAUSE:
            if (!pressed)
            {
                scene->parent_scene->time_scale = 1.0f;
                scene->parent_scene->state_bits = 0b111;
                scene->state_bits = 0b000;
            }
        break;
        default:
        break;
    }
    return ACTION_CONSUMED;
}

void init_level_scene(LevelScene_t* scene)
{
    init_scene(&scene->scene, &level_scene_render_func, &level_do_action);
    init_entity_tag_map(&scene->scene.ent_manager, PLAYER_ENT_TAG, 4);

    scene->data.game_field_size = (Vector2){ARENA_WIDTH, ARENA_HEIGHT};
    scene->data.game_viewport = LoadRenderTexture(ARENA_WIDTH, ARENA_HEIGHT);
    scene->data.game_rec = (Rectangle){10, 10, ARENA_WIDTH, ARENA_HEIGHT};
    scene->scene.time_scale = 1.0f;

    
    memset(&scene->data.cam, 0, sizeof(Camera2D));
    scene->data.cam.zoom = 1.0;
    scene->data.cam.target = (Vector2){
        0,0
    };
    scene->data.cam.offset = (Vector2){
        0,0
    };
    
    create_player(&scene->scene.ent_manager);
    create_spawner(&scene->scene.ent_manager);
    update_entity_manager(&scene->scene.ent_manager);

    
    sc_array_add(&scene->scene.systems, &weapon_cooldown_system);
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &global_external_forces_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);
    sc_array_add(&scene->scene.systems, &invuln_update_system);
    sc_array_add(&scene->scene.systems, &hitbox_update_system);
    sc_array_add(&scene->scene.systems, &player_dir_reset_system);

    // Entities may be 'removed' here
    sc_array_add(&scene->scene.systems, &life_update_system);

    // Entities may be dead after here
    sc_array_add(&scene->scene.systems, &ai_update_system);
    sc_array_add(&scene->scene.systems, &homing_update_system);
    sc_array_add(&scene->scene.systems, &spawned_update_system);
    sc_array_add(&scene->scene.systems, &container_destroy_system);
    sc_array_add(&scene->scene.systems, &arena_render_func);

    sc_map_put_64(&scene->scene.action_map, KEY_W, ACTION_UP);
    sc_map_put_64(&scene->scene.action_map, KEY_S, ACTION_DOWN);
    sc_map_put_64(&scene->scene.action_map, KEY_A, ACTION_LEFT);
    sc_map_put_64(&scene->scene.action_map, KEY_D, ACTION_RIGHT);
    sc_map_put_64(&scene->scene.action_map, KEY_ONE, ACTION_SELECT1);
    sc_map_put_64(&scene->scene.action_map, KEY_TWO, ACTION_SELECT2);
    sc_map_put_64(&scene->scene.action_map, KEY_THREE, ACTION_SELECT3);
    sc_map_put_64(&scene->scene.action_map, KEY_FOUR, ACTION_SELECT4);
    sc_map_put_64(&scene->scene.action_map, MOUSE_BUTTON_RIGHT, ACTION_BOOST);
    sc_map_put_64(&scene->scene.action_map, MOUSE_BUTTON_LEFT, ACTION_SHOOT);
    sc_map_put_64(&scene->scene.action_map, KEY_P, ACTION_PAUSE);

    scene->scene.subscene->type = SUB_SCENE;
    init_scene(scene->scene.subscene, NULL, &shop_do_action);

    scene->scene.subscene->parent_scene = &scene->scene;
    scene->scene.subscene->subscene = NULL;
    scene->scene.time_scale = 1.0f;
    scene->scene.subscene_pos = (Vector2){10 + ARENA_WIDTH + 20, 10};
    sc_array_add(&scene->scene.subscene->systems, &shop_render_func);

    ShopScene_t* shop_scene = (ShopScene_t*)scene->scene.subscene;
    shop_scene->data.shop_viewport = LoadRenderTexture(400, ARENA_HEIGHT - 150);
    shop_scene->data.shop_rec = (Rectangle){0, 0, 400, ARENA_HEIGHT - 150};
}

void free_level_scene(LevelScene_t* scene)
{
    UnloadRenderTexture(scene->data.game_viewport); // Unload render texture
                                                    //
    ShopScene_t* shop_scene = (ShopScene_t*)scene->scene.subscene;
    UnloadRenderTexture(shop_scene->data.shop_viewport); // Unload render texture
    free_scene(&scene->scene);
    free_scene(scene->scene.subscene);
}
