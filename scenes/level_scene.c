#include "EC.h"
#include "actions.h"
#include "assets_tag.h"
#include "engine.h"
#include "level_ent.h"
#include "game_systems.h"
#include "constants.h"
#include "raylib.h"
#include "raymath.h"
#include "mempool.h"
#include "scenes.h"
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
            sprintf(mem_stats, "Growth: %u", p_pstate->collected);
            DrawText(
                mem_stats, scene->subscene_pos.x,
                scene->subscene_pos.y + shop_scene->data.shop_rec.height + 106,
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
            else if (p_ent->m_tag == COLLECT_ENT_TAG)
            {
                c = GOLD;
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
    char buffer[8];
    BeginTextureMode(data->shop_viewport);
        ClearBackground(RAYWHITE);
        if (scene->state_bits & RENDER_BIT)
        {
            for (uint8_t i = 0; i < 8; ++i)
            {
                DrawRectangleRec(
                    data->ui.upgrades[i].button.box,
                    data->ui.upgrades[i].button.enabled ? BLACK: GRAY);
                Vector2 center = {
                    data->ui.upgrades[i].button.box.x + data->ui.upgrades[i].button.box.width,
                    data->ui.upgrades[i].button.box.y + data->ui.upgrades[i].button.box.height / 2,
                };

                center.x += 20;
                if (data->ui.upgrades[i].show_dots)
                {
                    const int8_t bought = data->ui.upgrades[i].item->cap - data->ui.upgrades[i].item->remaining;
                    for (int8_t j = 0; j < bought; ++j)
                    {
                        DrawCircleV(center, data->ui.dot_size, BLACK);
                        center.x += data->ui.upgrades[i].dot_spacing;
                    }
                    for (int8_t j = 0; j < data->ui.upgrades[i].item->remaining; ++j)
                    {
                        DrawCircleLinesV(center, data->ui.dot_size, BLACK);
                        center.x += data->ui.upgrades[i].dot_spacing;
                    }
                    center.x -= data->ui.upgrades[i].dot_spacing;
                    center.x += 20;
                }

                sprintf(buffer, "%04u", data->ui.upgrades[i].item->cost);
                DrawText(buffer, center.x, center.y - (36 >> 1), 36, BLACK);
            }

            DrawRectangleLinesEx(data->ui.desc_box, 3, BLACK);
        }

    EndTextureMode();
}

static ActionResult shop_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    ShopSceneData* data = &(((ShopScene_t*)scene)->data);

    switch(action)
    {
        case ACTION_SHOOT:
            if (!pressed)
            {
                Entity_t* p_player;
                sc_map_foreach_value(&scene->parent_scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
                {
                    CPlayerState_t* p_pstate = get_component(p_player, CPLAYERSTATE_T);
                    for (uint8_t i = 0; i < 8; ++i)
                    {
                        if (CheckCollisionPointRec(scene->mouse_pos, data->ui.upgrades[i].button.box))
                        {
                            if (p_pstate->collected >= data->ui.upgrades[i].item->cost)
                            {
                                if (data->ui.upgrades[i].item->remaining > 0)
                                {
                                    data->ui.upgrades[i].item->remaining--;
                                    p_pstate->collected -= data->ui.upgrades[i].item->cost;
                                    if (data->ui.upgrades[i].item->remaining == 0)
                                    {
                                        data->ui.upgrades[i].button.enabled = false;
                                    }
                                }
                                else if (data->ui.upgrades[i].item->cap < 0)
                                {
                                    p_pstate->collected -= data->ui.upgrades[i].item->cost;
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
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

static void generate_shop_UI(ShopSceneData* data)
{
    const int padding = 5;
    const int DOT_SPACING = 60;
    const int DESCRPTION_BOX_HEIGHT = 130;
    const int width = data->shop_rec.width - padding * 2;
    const int height = data->shop_rec.height - padding * 2;
    const int upgrade_height = (height - DESCRPTION_BOX_HEIGHT) / 6;

    Vector2 pos = {padding,padding};
            // Stats Upgrades: Hardcoded to 4 upgrades lol
    for (uint8_t i = 0; i < 4; ++i)
    {
        data->ui.upgrades[i].button.box.x = pos.x;
        data->ui.upgrades[i].button.box.y = pos.y;
        data->ui.upgrades[i].button.box.width = data->ui.icon_size;
        data->ui.upgrades[i].button.box.height = data->ui.icon_size;
        data->ui.upgrades[i].dot_spacing = DOT_SPACING;
        data->ui.upgrades[i].show_dots = true;
        data->ui.upgrades[i].button.enabled = true;

        pos.y += (upgrade_height > data->ui.icon_size) ? upgrade_height : data->ui.icon_size; 
    }

    data->ui.upgrades[4].button.box.x = padding;
    data->ui.upgrades[4].button.box.y = pos.y;
    data->ui.upgrades[4].button.box.width = data->ui.icon_size;
    data->ui.upgrades[4].button.box.height = data->ui.icon_size;
    data->ui.upgrades[4].dot_spacing = DOT_SPACING;
    data->ui.upgrades[4].show_dots = false;
    data->ui.upgrades[4].button.enabled = true;

    data->ui.upgrades[5].button.box.x = (width >> 1);
    data->ui.upgrades[5].button.box.y = pos.y;
    data->ui.upgrades[5].button.box.width = data->ui.icon_size;
    data->ui.upgrades[5].button.box.height = data->ui.icon_size;
    data->ui.upgrades[5].dot_spacing = DOT_SPACING;
    data->ui.upgrades[5].show_dots = false;
    data->ui.upgrades[5].button.enabled = true;

    pos.y += (upgrade_height > data->ui.icon_size) ? upgrade_height : data->ui.icon_size; 

    data->ui.upgrades[6].button.box.x = padding;
    data->ui.upgrades[6].button.box.y = pos.y;
    data->ui.upgrades[6].button.box.width = data->ui.icon_size;
    data->ui.upgrades[6].button.box.height = data->ui.icon_size;
    data->ui.upgrades[6].dot_spacing = DOT_SPACING;
    data->ui.upgrades[6].show_dots = false;
    data->ui.upgrades[6].button.enabled = true;

    data->ui.upgrades[7].button.box.x = (width >> 1);
    data->ui.upgrades[7].button.box.y = pos.y;
    data->ui.upgrades[7].button.box.width = data->ui.icon_size;
    data->ui.upgrades[7].button.box.height = data->ui.icon_size;
    data->ui.upgrades[7].dot_spacing = DOT_SPACING;
    data->ui.upgrades[7].show_dots = false;
    data->ui.upgrades[7].button.enabled = true;

    data->ui.upgrades[0].item = &data->store.firerate_upgrade;
    data->ui.upgrades[1].item = &data->store.projspeed_upgrade;
    data->ui.upgrades[2].item = &data->store.damage_upgrade;
    data->ui.upgrades[3].item = &data->store.health_upgrade;
    data->ui.upgrades[4].item = &data->store.full_heal;
    data->ui.upgrades[5].item = &data->store.escape;
    data->ui.upgrades[6].item = &data->store.thumper;
    data->ui.upgrades[7].item = &data->store.maws;

    pos.y += data->ui.icon_size + padding; 
    data->ui.desc_box.x = padding;
    data->ui.desc_box.y = pos.y;
    data->ui.desc_box.width = width;
    data->ui.desc_box.height = height - pos.y;
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
    sc_array_add(&scene->scene.systems, &magnet_update_system);
    sc_array_add(&scene->scene.systems, &global_external_forces_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);

    sc_array_add(&scene->scene.systems, &invuln_update_system);
    sc_array_add(&scene->scene.systems, &money_collection_system);
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

    shop_scene->data.ui.icon_size = 75;
    shop_scene->data.ui.dot_size = 10;
    shop_scene->data.ui.pos = (Vector2){50, 50};

    shop_scene->data.store = (UpgradeStoreInventory){
        .firerate_upgrade = {50,3,3},
        .projspeed_upgrade = {50,3,3},
        .damage_upgrade = {50,3,2},
        .health_upgrade = {50,3,3},
        .full_heal = {50,-1,-1},
        .escape = {500,1,1},
        .thumper = {100,1,1},
        .maws = {200,1,1},
    };
    generate_shop_UI(&shop_scene->data);
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
