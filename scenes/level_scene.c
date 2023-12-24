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

#define TEXT_COLOUR WHITE
#define BG_COLOUR BLACK
#define ARENA_COLOUR (Color){30,30,30,255}
#define THEME_COLOUR RED
#define SELECTION_COLOUR PINK

void restart_level_scene(LevelScene_t* scene);

static char* TEXT_DESCRIPTION[] = {
    "Fire rate",
    "Projectile speed",
    "Damage",
    "Max Health",
    "Full Heal",
    "Escape?",
    "Thumper",
    "Maws",
};

static ActionResult level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    if (action == ACTION_RESTART && !pressed)
    {
        restart_level_scene((LevelScene_t*)scene);
        return ACTION_CONSUMED;
    }
    LevelSceneData_t* data = &((LevelScene_t*)scene)->data;
            
    Entity_t* p_player;
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
                if (data->game_state == GAME_STARTING)
                {
                    create_spawner(&scene->ent_manager);
                    data->game_state = GAME_PLAYING;
                }
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
        ClearBackground(BG_COLOUR);
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
        DrawText(mem_stats, data->game_rec.x + 10, data->game_rec.y, 12, TEXT_COLOUR);
        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_ent)
        {
            CLifeTimer_t* p_life = get_component(p_ent, CLIFETIMER_T);
            sprintf(mem_stats, "HP: %u/%u ( %.2f )", p_life->current_life, p_life->max_life, p_life->corruption);
            DrawText(
                mem_stats, scene->subscene_pos.x,
                scene->subscene_pos.y + shop_scene->data.shop_rec.height + 10,
                32, TEXT_COLOUR
            );

            CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
            sprintf(mem_stats, "Redirection: %.2f", p_pstate->boost_cooldown);
            DrawText(
                mem_stats, scene->subscene_pos.x,
                scene->subscene_pos.y + shop_scene->data.shop_rec.height + 42,
                32, TEXT_COLOUR
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
                32, TEXT_COLOUR
            );
            sprintf(mem_stats, "Growth: %u", p_pstate->collected);
            DrawText(
                mem_stats, scene->subscene_pos.x,
                scene->subscene_pos.y + shop_scene->data.shop_rec.height + 106,
                32, TEXT_COLOUR
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
        ClearBackground(ARENA_COLOUR);
        BeginMode2D(data->cam);

        if (data->game_state == GAME_STARTING)
        {
            DrawText("Press Shoot to Begin", data->game_field_size.x / 4, data->game_field_size.y / 2 - (36 >> 1), 36, TEXT_COLOUR);
        }
        else if (data->game_state == GAME_ENDED)
        {
            DrawText("You have perished.", data->game_field_size.x / 4 , data->game_field_size.y / 2- (36 >> 1), 36, TEXT_COLOUR);
        }

        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
            if (p_cspr != NULL)
            {
                Sprite_t* spr = p_cspr->sprites[p_cspr->current_idx];
                if (spr!= NULL)
                {
                    Vector2 pos = Vector2Add(p_ent->position, p_cspr->offset);
                    draw_sprite(spr, p_cspr->current_frame, pos, p_cspr->rotation, p_cspr->flip_x);
                }
            }
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
                DrawLineEx(p_ent->position, look_dir, 2, (Color){255,255,255,64});
            }
        }
        Vector2 raw_mouse_pos = GetMousePosition();
        raw_mouse_pos = Vector2Subtract(raw_mouse_pos, (Vector2){data->game_rec.x, data->game_rec.y});
        DrawCircleV(raw_mouse_pos, 2, TEXT_COLOUR);

        EndMode2D();

    EndTextureMode();
}

void shop_render_func(Scene_t* scene)
{
    ShopSceneData* data = &(((ShopScene_t*)scene)->data);
    char buffer[8];
    BeginTextureMode(data->shop_viewport);
        ClearBackground(BG_COLOUR);
        if (scene->state_bits & RENDER_BIT)
        {
            for (uint8_t i = 0; i < 8; ++i)
            {
                Rectangle box = data->ui.upgrades[i].button.box;
                DrawRectangleRec(
                    box,
                    data->ui.upgrades[i].button.enabled ? THEME_COLOUR : GRAY
                );
                if (
                    data->ui.upgrades[i].button.hover
                    && data->ui.upgrades[i].button.enabled)
                {
                    box.x += 5;
                    box.y += 5;
                    box.width -= 10;
                    box.height -= 10;
                    DrawRectangleRec(box, SELECTION_COLOUR);
                }

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
                        DrawCircleV(center, data->ui.dot_size, TEXT_COLOUR);
                        center.x += data->ui.upgrades[i].dot_spacing;
                    }
                    for (int8_t j = 0; j < data->ui.upgrades[i].item->remaining; ++j)
                    {
                        DrawCircleLinesV(center, data->ui.dot_size, TEXT_COLOUR);
                        center.x += data->ui.upgrades[i].dot_spacing;
                    }
                    center.x -= data->ui.upgrades[i].dot_spacing;
                    center.x += 20;
            }

                sprintf(buffer, "%04u", data->ui.upgrades[i].item->cost);
                DrawText(buffer, center.x, center.y - (36 >> 1), 36, TEXT_COLOUR);
            }

            DrawRectangleLinesEx(data->ui.desc_box, 3, THEME_COLOUR);
            DrawText(
                data->ui.desc_text,
                data->ui.desc_box.x + 10,
                data->ui.desc_box.y +(data->ui.desc_box.height / 2) - (36 >> 1),
                36, TEXT_COLOUR
            );
        }
        else
        {
            Vector2 text_pos = {
                .x = 5,
                .y = data->shop_rec.height / 2
            };
            DrawText("Press P to Pause", text_pos.x, text_pos.y - (36 >> 1), 36, TEXT_COLOUR);
        }

    EndTextureMode();
}

static void game_over_check(Scene_t* scene)
{
    if (sc_map_size_64v(&scene->ent_manager.entities_map[PLAYER_ENT_TAG]) == 0)
    {
        LevelSceneData_t* data = &((LevelScene_t*)scene)->data;
        data->game_state = GAME_ENDED;
        Entity_t* p_ent;
        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            if (get_component(p_ent, CSPAWNER_T))
            {
                remove_component(p_ent, CSPAWNER_T);
            }
        }
    }
}

static void shop_check_mouse(Scene_t* scene)
{
    ShopSceneData* data = &(((ShopScene_t*)scene)->data);

    // Reset
    data->ui.desc_text = "";
    for (uint8_t i = 0; i < 8; ++i)
    {
            data->ui.upgrades[i].button.hover = false;
    }

    for (uint8_t i = 0; i < 8; ++i)
    {
        if (CheckCollisionPointRec(scene->mouse_pos, data->ui.upgrades[i].button.box))
        {
            data->ui.desc_text = TEXT_DESCRIPTION[i];
            data->ui.upgrades[i].button.hover = true;
            break;
        }
    }
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
                    CWeaponStore_t* p_weapons = get_component(p_player, CWEAPONSTORE_T);
                    CLifeTimer_t* p_life = get_component(p_player, CLIFETIMER_T);
                    for (uint8_t i = 0; i < 8; ++i)
                    {
                        if (CheckCollisionPointRec(scene->mouse_pos, data->ui.upgrades[i].button.box))
                        {
                            bool purchased = false;
                            if (p_pstate->collected >= data->ui.upgrades[i].item->cost)
                            {
                                if (data->ui.upgrades[i].item->remaining > 0)
                                {
                                    purchased = true;
                                    data->ui.upgrades[i].item->remaining--;
                                    if (data->ui.upgrades[i].item->remaining == 0)
                                    {
                                        data->ui.upgrades[i].button.enabled = false;
                                    }
                                }
                                else if (data->ui.upgrades[i].item->cap < 0)
                                {
                                    purchased = true;
                                }

                                if (purchased)
                                {
                                    p_pstate->collected -= data->ui.upgrades[i].item->cost;
                                    data->ui.upgrades[i].item->cost +=  data->ui.upgrades[i].item->cost_increment;
                                    if (data->ui.upgrades[i].item->cost > data->ui.upgrades[i].item->cost_cap)
                                    {
                                        data->ui.upgrades[i].item->cost =  data->ui.upgrades[i].item->cost_cap;
                                    }

                                    switch (i)
                                    {
                                        case 0:
                                        case 1:
                                        case 2:
                                            p_weapons->modifier[i]++;
                                        break;
                                        case 3:
                                            p_life->current_life += 20;
                                            p_life->max_life += 20;
                                        break;
                                        case 4:
                                            p_life->current_life = p_life->max_life;
                                        break;
                                        case 5:
                                            // Game ending stuff
                                        break;
                                        case 6:
                                            p_weapons->unlocked[1] = true;
                                        break;
                                        case 7:
                                            p_weapons->unlocked[2] = true;
                                        break;
                                        default:
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        break;
        case ACTION_RESTART:
            restart_level_scene((LevelScene_t*)scene->parent_scene);
        //fallthrough intended
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

void restart_level_scene(LevelScene_t* scene)
{
    Entity_t* p_ent;
    sc_map_foreach_value(&scene->scene.ent_manager.entities, p_ent)
    {
        remove_entity(&scene->scene.ent_manager, p_ent->m_id);
    }
    update_entity_manager(&scene->scene.ent_manager);

    Entity_t* player = create_player(&scene->scene.ent_manager);
    player->position = Vector2Scale(scene->data.game_field_size, 0.5f);
    //create_spawner(&scene->scene.ent_manager);
    update_entity_manager(&scene->scene.ent_manager);

    ShopScene_t* shop_scene = (ShopScene_t*)scene->scene.subscene;
    shop_scene->data.store = (UpgradeStoreInventory){
        .firerate_upgrade = {100, 3, 3, 125,1000},
        .projspeed_upgrade = {75, 3, 3, 75, 1000},
        .damage_upgrade = {50, 3, 3, 100, 1000},
        .health_upgrade = {100, 3, 3, 150, 1000},
        .full_heal = {50, -1, -1, 50, 500},
        .escape = {2000, 1, 1, 0, 5000},
        .thumper = {200, 1, 1, 0, 1000},
        .maws = {200, 1, 1, 0, 1000},
    };
    scene->data.game_state = GAME_STARTING;
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
    sc_array_add(&scene->scene.systems, &game_over_check);
    sc_array_add(&scene->scene.systems, &sprite_animation_system);
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
    sc_map_put_64(&scene->scene.action_map, KEY_L, ACTION_RESTART);

    scene->scene.subscene->type = SUB_SCENE;
    init_scene(scene->scene.subscene, NULL, &shop_do_action);

    scene->scene.subscene->parent_scene = &scene->scene;
    scene->scene.subscene->subscene = NULL;
    scene->scene.time_scale = 1.0f;
    scene->scene.subscene_pos = (Vector2){10 + ARENA_WIDTH + 20, 10};
    sc_array_add(&scene->scene.subscene->systems, &shop_check_mouse);
    sc_array_add(&scene->scene.subscene->systems, &shop_render_func);

    ShopScene_t* shop_scene = (ShopScene_t*)scene->scene.subscene;
    shop_scene->data.shop_viewport = LoadRenderTexture(400, ARENA_HEIGHT - 150);
    shop_scene->data.shop_rec = (Rectangle){0, 0, 400, ARENA_HEIGHT - 150};

    shop_scene->data.ui.icon_size = 75;
    shop_scene->data.ui.dot_size = 10;
    shop_scene->data.ui.pos = (Vector2){50, 50};

    generate_shop_UI(&shop_scene->data);

    restart_level_scene(scene);
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
