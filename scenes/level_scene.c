#include "EC.h"
#include "actions.h"
#include "assets_tag.h"
#include "engine.h"
#include "level_ent.h"
#include "game_systems.h"
#include "ai_functions.h"
#include "constants.h"
#include "raylib.h"
#include "raymath.h"
#include "mempool.h"
#include "scenes.h"
#include <stdio.h>
#include <stdlib.h>

#define TEXT_COLOUR WHITE
#define BG_COLOUR BLACK
#define ARENA_COLOUR (Color){20,20,20,255}
#define THEME_COLOUR RED
#define SELECTION_COLOUR PINK
#define WEP_ICON_SPACING 150
static const int total_icons_width = (96 + WEP_ICON_SPACING) * 4;

void restart_level_scene(LevelScene_t* scene);

static char* TEXT_DESCRIPTION[] = {
    "Fire rate",
    "Projectile Speed",
    "Max Health",
    "Full Heal",
    "Void Particle",
    "Thumper",
    "Maws",
    "Flux",
};

static ActionResult level_do_action(Scene_t* scene, ActionType_t action, bool pressed)
{
    if (action == ACTION_RESTART && !pressed)
    {
        restart_level_scene((LevelScene_t*)scene);
        return ACTION_CONSUMED;
    }
    //LevelSceneData_t* data = &((LevelScene_t*)scene)->data;
            
    Entity_t* p_player;
    sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
    {
        CPlayerState_t* p_playerstate = get_component(p_player, CPLAYERSTATE_T);
        CWeaponStore_t* p_weaponstore = get_component(p_player, CWEAPONSTORE_T);
        CWeapon_t* p_weapon = get_component(p_player, CWEAPON_T);
        uint8_t new_weapon = p_weapon->weapon_idx;
        switch(action)
        {
            case ACTION_MOVE:
                p_playerstate->moving |= (pressed) ? 1 : 0;
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
                    play_sfx(scene->engine, PAUSE_SFX, true);
                    scene->time_scale = 0.0f;
                    scene->state_bits = 0b110;
                    scene->subscene->state_bits = 0b111;
                    // This only works because time stops in the store
                    ShopScene_t* shop_scene = (ShopScene_t*)scene->subscene;
                    shop_scene->data.curr_money = p_playerstate->collected;
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
                p_weapon->hold_timer = 0.0f;
                p_weapon->selected = false;
                p_weaponstore->weapons[p_weapon->weapon_idx] = *p_weapon;
                p_weaponstore->weapons[new_weapon].selected = true;
                *p_weapon = p_weaponstore->weapons[new_weapon];
                CSprite_t* p_cspr = get_component(p_player, CSPRITE_T);
                if (p_cspr != NULL)
                {
                    p_cspr->current_frame = new_weapon;
                }
                play_sfx(scene->engine, WEAPON1_HIT_SFX + new_weapon, true);
            }
        }
    }
    return ACTION_PROPAGATE;
}
#define STATS_Y_OFFSET 0
static void level_scene_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    ShopScene_t* shop_scene = (ShopScene_t*)scene->subscene;
    
    Rectangle draw_rec = data->game_rec;
    draw_rec.x = 0;
    draw_rec.y = 0;
    draw_rec.height *= -1;

    Entity_t* player_ent = NULL;
    // Due to a quirk in the looping, player_ent will be given a value
    // before the loop starts.
    // However, the loop will terminate on an empty map as expected.
    // Yet, the value is assigned even on an empty map
    // Hence, need to check for size before looping for this case
    if (sc_map_size_64v(&scene->ent_manager.entities_map[PLAYER_ENT_TAG]))
    {
        // Obtain the first player
        sc_map_foreach_value(&scene->ent_manager.entities_map[PLAYER_ENT_TAG], player_ent)
        {
            break;
        }
    }

    Image stat_view = GenImageColor(
        total_icons_width, 180,
        BG_COLOUR
    );
    Image mask = GenImageColor(
        total_icons_width, 180,
        (Color){0,0,0,255}
    );
    if (player_ent != NULL)
    {
        CWeaponStore_t* p_weaponstore = get_component(player_ent, CWEAPONSTORE_T);
        CWeapon_t* p_weapon = get_component(player_ent, CWEAPON_T);

        const int icon_width = data->weapon_icons.width;
        const int icon_height = data->weapon_icons.height / 4;
        const char* WEAPON_KEYS[4] = {"A/J","S/K","D/L","F/;"};
        for (uint8_t i = 0; i < p_weaponstore->n_weapons; ++i)
        {
            if (!p_weaponstore->unlocked[i]) continue;

            int x_pos = (icon_width + WEP_ICON_SPACING) * i;
            ImageDraw(
                &stat_view, data->weapon_icons,
                (Rectangle){0, icon_height*i, icon_width, icon_height},
                (Rectangle){x_pos, 0, icon_width, icon_height},
                (i == p_weapon->weapon_idx) ? WHITE : GRAY
            );
            ImageDrawRectangleRec(
                &mask,
                (Rectangle){x_pos, 0, icon_width, icon_height},
                (Color){64,64,64,128}
            );
            ImageDrawRectangleRec(
                &mask,
                (Rectangle){x_pos + icon_width + 10, 0, 72, icon_height},
                (Color){255, 255 , 255, 255}
            );
            ImageDrawText(&stat_view, WEAPON_KEYS[i], x_pos + icon_width + 10, 24 >> 1, 24, WHITE);

            float cooldown_timer = 1;
            float current_cooldown = 0;
            if (i == p_weapon->weapon_idx)
            {
                cooldown_timer = 1.0f / (p_weapon->fire_rate  * (1 + p_weapon->modifiers[0] * 0.1));
                current_cooldown = p_weapon->cooldown_timer;
            }
            else
            {
                cooldown_timer = 1.0f / (p_weaponstore->weapons[i].fire_rate  * (1 + p_weaponstore->weapons[i].modifiers[0] * 0.1));
                current_cooldown = p_weaponstore->weapons[i].cooldown_timer;
            }
            Color wep_col = (current_cooldown > 0.0f) ? RED : WHITE;
            int show_height = icon_width * (cooldown_timer - current_cooldown) / cooldown_timer;
            ImageDrawRectangleRec(
                &mask,
                (Rectangle){(icon_width + WEP_ICON_SPACING) * i, 0, show_height, icon_height},
                wep_col
            );
        }
    }
    ImageAlphaMask(&stat_view, mask);
    UpdateTexture(data->stat_view.texture, stat_view.data);

    BeginDrawing();
        ClearBackground(BG_COLOUR);
        DrawTextureRec(
            data->game_viewport.texture,
            draw_rec,
            (Vector2){data->game_rec.x, data->game_rec.y},
            WHITE
        );


        static char mem_stats[512];
        print_mempool_stats(mem_stats);

        sprintf(mem_stats, "%u %u %u", GetFPS(), get_num_of_free_entities(), get_number_of_free_emitter(&scene->part_sys));
        DrawText(mem_stats, data->game_rec.x + 10, data->game_rec.y + data->game_rec.height + 12, 12, TEXT_COLOUR);
        const int PLAYER_STAT_FONT = 24;

        draw_rec = shop_scene->data.shop_rec;
        draw_rec.x = shop_scene->data.shop_rec.x + scene->subscene_pos.x - 2;
        draw_rec.y = shop_scene->data.shop_rec.y + scene->subscene_pos.y - 2;
        draw_rec.width += 4;
        draw_rec.height += 4;
        DrawRectangleRec(draw_rec, RED);

        draw_rec = shop_scene->data.shop_rec;
        draw_rec.x = 0;
        draw_rec.y = 0;
        draw_rec.height *= -1;
        DrawTextureRec(
            shop_scene->data.shop_viewport.texture,
            draw_rec,
            (Vector2){
                shop_scene->data.shop_rec.x + scene->subscene_pos.x,
                shop_scene->data.shop_rec.y + scene->subscene_pos.y
            },
            WHITE
        );
        if (player_ent != NULL)
        {
            CPlayerState_t* p_pstate = get_component(player_ent, CPLAYERSTATE_T);
            sprintf(mem_stats, "Essence: %u", p_pstate->collected);
            DrawText(
                mem_stats, scene->subscene_pos.x,
                shop_scene->data.shop_rec.y +  shop_scene->data.shop_rec.height + scene->subscene_pos.y + (PLAYER_STAT_FONT >> 1),
                PLAYER_STAT_FONT, SKYBLUE
            );

            CLifeTimer_t* p_life = get_component(player_ent, CLIFETIMER_T);
            const int HEALTH_LENGTH = p_life->max_life * 1.0f / MAXIMUM_HEALTH * shop_scene->data.shop_rec.width;

            Vector2 bar_pos = (Vector2){
                shop_scene->data.shop_rec.x + scene->subscene_pos.x,
                scene->subscene_pos.y - 45
            };
            DrawRectangle(
                bar_pos.x, bar_pos.y,
                HEALTH_LENGTH * p_pstate->boost_cooldown / BOOST_COOLDOWN, 32 + 3, YELLOW
            );
            DrawRectangle(
                bar_pos.x + 3, bar_pos.y + 3,
                HEALTH_LENGTH - 10, 28, GRAY
            );

            {
                static uint8_t health_flash = 0;
                static float flash_timing = 0.0f;
                if (p_life->current_life <= CRIT_HEALTH)
                {
                    flash_timing += scene->delta_time;
                    if (flash_timing >= 0.2f)
                    {
                        health_flash = !health_flash;
                        flash_timing -= 0.2f;
                    }
                }
                else
                {
                    health_flash = false;
                    flash_timing = 0.0f;
                }
                DrawRectangle(
                    bar_pos.x + 3, bar_pos.y + 3,
                    //data->game_rec.x, data->game_rec.y - 32 - 5,
                    (HEALTH_LENGTH - 10) * p_life->current_life * 1.0f / p_life->max_life, 28,
                    (health_flash) ? WHITE : RED
                );
            }

            //stat_height += 40 + 5;

            DrawTextureRec(
                data->stat_view.texture,
                (Rectangle){0,0,stat_view.width,stat_view.height},
                (Vector2){
                    //scene->subscene_pos.x,
                    //stat_height,
                    //data->game_rec.x + data->game_rec.width - stat_view.width,
                    ARENA_START_X + 150,
                    data->game_rec.y - 45 - 10 // Hardcode the height
                },
                WHITE
            );
        }
        draw_rec.height = -data->credits_view.texture.height;
        DrawTextureRec(
            data->credits_view.texture,
            draw_rec,
            (Vector2){
                shop_scene->data.shop_rec.x + scene->subscene_pos.x,
                shop_scene->data.shop_rec.y + scene->subscene_pos.y + shop_scene->data.shop_rec.height + 50
            },
            WHITE
        );
        Vector2 raw_mouse_pos = GetMousePosition();
        Sprite_t* spr = get_sprite(&scene->engine->assets, "crosshair");
        draw_sprite(spr, 0, raw_mouse_pos, 0.0f, false, WHITE);
    EndDrawing();
    UnloadImage(stat_view);
    UnloadImage(mask);
}

#define N_CIRCS 64
struct PulseCircle
{
    float R;
    float phase;
    float rate;
};
struct PulseCircle circs[N_CIRCS] = {0};

static void arena_render_func(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    Entity_t* p_ent;
    
    for (int i = 0; i < N_CIRCS; ++i)
    {
        circs[i].phase += scene->delta_time * circs[i].rate;
        if (circs[i].phase > 2 * PI)
        {
            circs[i].phase -= 2 * PI;
        }
    }

    if (data->game_state == GAME_PLAYING)
    {
        data->survival_timer.fractional += scene->delta_time;
        if (data->survival_timer.fractional > 1.0f)
        {
            data->survival_timer.fractional -= 1.0f;
            data->survival_timer.seconds++;
        }
        if (data->survival_timer.seconds == 60)
        {
            data->survival_timer.seconds -= 60;
            data->survival_timer.minutes++;
        }
    }
    const int RING_MAX_DIAMETER = (data->game_field_size.x > data->game_field_size.y) ? 
        data->game_field_size.y : data->game_field_size.x;

    const int RING_RADIUS_INCREMENT = (RING_MAX_DIAMETER / 2) / (MAX_RANK);
    BeginTextureMode(data->game_viewport);
        ClearBackground(ARENA_COLOUR);
        BeginMode2D(data->camera.cam);


        CSpawner_t* p_spawner;
        unsigned int ent_idx;
        sc_map_foreach(&scene->ent_manager.component_map[CSPAWNER_T], ent_idx, p_spawner)
        {
            // Not going into the final game
            //static char buffer[64];
            //sprintf(buffer, "Spawned %u", p_spawner->data.spawned);
            //DrawText(buffer, 0, data->game_field_size.y - 32, 32, WHITE);
            Vector2 center = Vector2Scale(data->game_field_size, 0.5f);
            //for (uint8_t i = 0; i < lit_ring; i++)
            //{
            //    DrawCircleLinesV(center, radius, (Color){255,255,255,72});
            //    radius += RING_RADIUS_INCREMENT;
            //}
            
            DrawCircleV(
                center,
                RING_RADIUS_INCREMENT * p_spawner->data.rank
                + RING_RADIUS_INCREMENT * p_spawner->data.rank_counter / p_spawner->data.next_rank_count
                ,
                (Color){255,255,255,32}
            );
            int radius = 0;
            for (uint8_t i = 0; i < MAX_RANK - 1; i++)
            {
                DrawCircleLinesV(center, radius, (Color){255,255,255,16});
                radius += RING_RADIUS_INCREMENT;
            }
            DrawCircleLinesV(center, radius, (Color){255, 0, 0,64});
            break;
        }

        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            if (p_ent->m_tag == UTIL_ENT_TAG) continue;

            CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
            Sprite_t* spr = NULL;
            if (p_cspr != NULL)
            {
                spr = p_cspr->sprites[p_cspr->current_idx];
                if (spr!= NULL)
                {
                    if (p_ent->m_tag == ENEMY_ENT_TAG)
                    {
                        p_cspr->rotation += p_cspr->rotation_speed * scene->time_scale * scene->delta_time;
                    }
                }
            }

            CHitBoxes_t* p_hitbox = get_component(p_ent, CHITBOXES_T);
            if (p_hitbox != NULL && p_hitbox->type == HITBOX_RAY)
            {
                DrawLineV(
                    p_ent->position,
                    Vector2Add(
                        p_ent->position,
                        Vector2Scale(p_hitbox->dir, 10000)
                    ),
                    WHITE
                );
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
                c = SKYBLUE;
                CMoney_t* c_money = get_component(p_ent, CMONEY_T);
                if (c_money != NULL && c_money->value > 50)
                {
                    c = GOLD;
                }
            }
            
            Vector2 spr_positions[4];
            uint8_t n_pos = 1;
            spr_positions[0] = p_ent->position;

            CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);

            if (p_ct != NULL && p_ct->edge_b == EDGE_WRAPAROUND)
            {
                
                int8_t flip_x = (int8_t)(p_ct->boundary_checks & 0b11) - 1;
                int8_t flip_y = (int8_t)((p_ct->boundary_checks >> 2) & 0b11) - 1;


                Vector2 clone_pos = p_ent->position;
                if (flip_x != 0 && flip_y != 0)
                {
                    clone_pos.x += flip_x * data->game_field_size.x;
                    spr_positions[1] = clone_pos;
                    clone_pos.y += flip_y * data->game_field_size.y;
                    spr_positions[2] = clone_pos;
                    clone_pos.x -= flip_x * data->game_field_size.x;
                    spr_positions[3] = clone_pos;
                    n_pos = 4;
                }
                else if (flip_x != 0)
                {
                    clone_pos.x += flip_x * data->game_field_size.x;
                    spr_positions[1] = clone_pos;
                    n_pos = 2;
                }
                else
                {
                    clone_pos.y += flip_y * data->game_field_size.y;
                    spr_positions[1] = clone_pos;
                    n_pos = 2;
                }
            }

            if (data->game_state != GAME_ENDED)
            {
                CPlayerState_t* p_pstate = get_component(p_ent, CPLAYERSTATE_T);
                Vector2 look_dir = {0};
                if (p_pstate != NULL)
                {
                    look_dir = p_pstate->aim_dir;
                }

                for (uint8_t i = 0; i < n_pos; i++)
                {
                    if (p_ent->m_tag == PLAYER_ENT_TAG)
                    {

                        DrawCircleV(spr_positions[i], p_ent->size, c);
                        float radius = (BOOST_COOLDOWN - p_pstate->boost_cooldown) / BOOST_COOLDOWN * 6;
                        Vector2 pos = Vector2Add(
                            spr_positions[i], 
                            Vector2Scale(p_pstate->aim_dir, -16)
                        );
                        DrawCircleV(pos, radius, (p_pstate->boost_cooldown > 0)? GRAY : WHITE);
                    }
                    else if (p_cspr == NULL)
                    {
                        DrawCircleV(spr_positions[i], p_ent->size, c);
                    }

                    if (spr!= NULL)
                    {
                        Vector2 pos = Vector2Add(spr_positions[i], p_cspr->offset);
                        if (p_ent->m_tag == ENEMY_ENT_TAG)
                        {
                            draw_sprite_scaled(spr, p_cspr->current_frame, pos, p_cspr->rotation, p_ent->size * 2 / spr->frame_size.x, p_cspr->colour);
                        }
                        else
                        {
                            draw_sprite(spr, p_cspr->current_frame, pos, p_cspr->rotation, p_cspr->flip_x, p_cspr->colour);
                        }
                    }

                    CWeapon_t* p_weapon = get_component(p_ent, CWEAPON_T);
                    if (p_weapon != NULL)
                    {
                        if (p_weapon->special_prop & 0x4 && p_weapon->hold_timer > 0)
                        {
                            float real_spread_range = p_weapon->spread_range * (1 - p_weapon->hold_timer * p_weapon->proj_speed / 150.0f);
                            if (real_spread_range < 0) real_spread_range = 0;
                            Vector2 spread_p = Vector2Rotate(look_dir, -real_spread_range);
                            DrawLineEx(
                                spr_positions[i],
                                Vector2Add(spr_positions[i], Vector2Scale(spread_p, 6000)),
                                2, (Color){255,255,255,32}
                            );
                            spread_p = Vector2Rotate(look_dir, real_spread_range);
                            DrawLineEx(
                                spr_positions[i],
                                Vector2Add(spr_positions[i], Vector2Scale(spread_p, 6000)),
                                2, (Color){255,255,255,32}
                            );
                        }
                    }

                    DrawLineEx(
                        spr_positions[i],
                        Vector2Add(spr_positions[i], Vector2Scale(look_dir, 64)),
                        2, (Color){255,255,255,64}
                    );
                }
            }
            // Wraparound display
            if (p_ent->m_tag == PLAYER_ENT_TAG)
            {
#define INDICATOR_SIZE 6
                if (p_ent->position.x < 127)
                {
                    DrawCircleV(
                        (Vector2){data->game_field_size.x - 16, p_ent->position.y},
                         INDICATOR_SIZE, (Color){255,255,255,255 - (p_ent->position.x * 2)}
                    );
                }
                else if (p_ent->position.x > data->game_field_size.x - 128)
                {
                    DrawCircleV(
                        (Vector2){16, p_ent->position.y},
                         INDICATOR_SIZE,
                         (Color){255,255,255,255 - (data->game_field_size.x - p_ent->position.x) * 2}
                    );
                }
                
                if (p_ent->position.y < 128)
                {
                    DrawCircleV(
                        (Vector2){p_ent->position.x, data->game_field_size.y - 16},
                         INDICATOR_SIZE, (Color){255,255,255,255 - (p_ent->position.y * 2)}
                    );
                }
                else if (p_ent->position.y > data->game_field_size.y - 128)
                {
                    DrawCircleV(
                        (Vector2){p_ent->position.x, 16},
                          INDICATOR_SIZE,
                         (Color){255,255,255,255 - (data->game_field_size.y - p_ent->position.y )* 2}
                    );
                }
            }
        }

        draw_particle_system(&scene->part_sys);

        if (data->game_state == GAME_STARTING)
        {
            DrawText("Void Particle: Post-Jam Edition", data->game_field_size.x / 16, data->game_field_size.y / 8 - (60 >> 1), 60, TEXT_COLOUR);
            DrawText("by SadPumpkin", data->game_field_size.x / 2 + 300, data->game_field_size.y / 8 + 36 + 5 - (20 >> 1), 20, TEXT_COLOUR);

            Sprite_t* spr = get_sprite(&scene->engine->assets, "ms_ctrl");
            draw_sprite_scaled(spr, 0, (Vector2){
                data->game_field_size.x * 3 / 4,data->game_field_size.y  / 2},
            0.0f, 1.5f, WHITE);
            spr = get_sprite(&scene->engine->assets, "kb_ctrl");
            draw_sprite_scaled(spr, 0, (Vector2){
                data->game_field_size.x / 4,data->game_field_size.y / 2 + 50},
            0.0f, 1.5f, WHITE);
            DrawText("Objective: Obtain the Void Particle from The Store", 40, data->game_field_size.y *3/ 4 - (36 >> 1), 36, TEXT_COLOUR);
            DrawText("Move about and mess around", 40, data->game_field_size.y *3/ 4  + 15 + (30 >> 1), 30, TEXT_COLOUR);
            DrawText("Once you are ready, SHOOT IT to Begin", 40, data->game_field_size.y *3/ 4  + 45 + (30 >> 1), 30, TEXT_COLOUR);
        }
        else if (data->game_state == GAME_ENDED)
        {
            const char* game_over_str;
            static char buf[64];
            if (!data->win_flag)
            {
                game_over_str = "You did not get the void particle.";
                sprintf(buf, "Survival Time: %u:%02u", data->survival_timer.minutes, data->survival_timer.seconds);
            }
            else
            {
                game_over_str = "You escaped with the void particle!";
                sprintf(buf, "Completion Time: %u:%02u", data->survival_timer.minutes, data->survival_timer.seconds);
            }

            DrawText(game_over_str, data->game_field_size.x / 8 , data->game_field_size.y / 2- (36 >> 1), 36, TEXT_COLOUR);
            DrawText(buf, data->game_field_size.x / 8 , data->game_field_size.y *3/4- (24 >> 1), 24, TEXT_COLOUR);
            DrawText("Thank you for playing!", data->game_field_size.x / 8 , data->game_field_size.y *8/10- (24 >> 1), 24, TEXT_COLOUR);
            DrawText("Press Y to begin a new cycle", data->game_field_size.x / 8 , data->game_field_size.y *9/10- (32 >> 1), 32, TEXT_COLOUR);

            if (data->endeffect_timer < 2000)
            {
                float alpha = 255;
                if (data->endeffect_timer > 1000)
                {
                    alpha = 255 - 255 * (data->endeffect_timer - 1000) * 1.0f/1000.0f;
                }
                DrawCircleV(data->endeffect_pos, data->endeffect_timer,(Color){255,255,255,alpha});
                data->endeffect_timer += 1500 * scene->delta_time;

            }
        }
        Vector2 center = {32, -10};
        for (int i = 0; i < N_CIRCS; ++i)
        {

            DrawCircleV(center, fabs(circs[i].R * sinf(circs[i].phase)), BLACK);
            center.y += data->game_field_size.y + 20;
            DrawCircleV(center, fabs(circs[i].R * sinf(circs[i].phase)), BLACK);
            center.y -= data->game_field_size.y + 20;
            center.x += circs[i].R;
        }

        center = (Vector2){-10, 32};
        for (int i = 0; i < N_CIRCS; ++i)
        {
            DrawCircleV(center, fabs(circs[i].R * sinf(circs[i].phase)), BLACK);
            center.x += data->game_field_size.x + 20;
            DrawCircleV(center, fabs(circs[i].R * sinf(circs[i].phase)), BLACK);
            center.x -= data->game_field_size.x + 20;

            center.y += circs[i].R;
        }

        draw_particle_system(&scene->part_sys);

        EndMode2D();

    EndTextureMode();
}

void shop_render_func(Scene_t* scene)
{
    const int UPGRADE_FONT_SIZE = 30;
    const int SPACING = 15;
    ShopSceneData* data = &(((ShopScene_t*)scene)->data);
    char buffer[8];
    BeginTextureMode(data->shop_viewport);
        ClearBackground(BG_COLOUR);
        if (scene->state_bits & RENDER_BIT)
        {
            for (uint8_t i = 0; i < N_UPGRADES; ++i)
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
                    data->ui.upgrades[i].button.box.x + data->ui.upgrades[i].button.box.width / 2,
                    data->ui.upgrades[i].button.box.y + data->ui.upgrades[i].button.box.height / 2,
                };

                if (data->ui.upgrades[i].spr != NULL)
                {
                    draw_sprite(
                        data->ui.upgrades[i].spr,
                        0, center, 0.0f, 0, WHITE
                    );
                }

                center.x += data->ui.upgrades[i].button.box.width / 2 + SPACING;
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
                    center.x += SPACING * 2;
                }

                sprintf(buffer, "%04u", data->ui.upgrades[i].item->cost);
                DrawText(
                    buffer, center.x, center.y - (UPGRADE_FONT_SIZE >> 1),
                    UPGRADE_FONT_SIZE,
                    (data->ui.upgrades[i].item->remaining && data->curr_money >= data->ui.upgrades[i].item->cost) ? TEXT_COLOUR : (Color){64,64,64,255});

            }

            DrawRectangleLinesEx(data->ui.desc_box, 3, THEME_COLOUR);
            DrawText(
                data->ui.desc_text,
                data->ui.desc_box.x + 10,
                data->ui.desc_box.y +(data->ui.desc_box.height / 2) - (UPGRADE_FONT_SIZE >> 1),
                UPGRADE_FONT_SIZE, TEXT_COLOUR
            );
            DrawText("Q/P to resume", data->ui.desc_box.x + 10,
                data->ui.desc_box.y + data->ui.desc_box.height + 10,
                24, TEXT_COLOUR);

        }
        else
        {
            Vector2 text_pos = {
                .x = 5,
                .y = data->shop_rec.height / 2
            };
            DrawText("Press Q or P", text_pos.x, text_pos.y, 30, TEXT_COLOUR);
            DrawText("to enter The Store", text_pos.x, text_pos.y + 30 + 2, 30, TEXT_COLOUR);
            Entity_t* p_player;
            sc_map_foreach_value(&scene->parent_scene->ent_manager.entities_map[PLAYER_ENT_TAG], p_player)
            {
                CPlayerState_t* p_pstate = get_component(p_player, CPLAYERSTATE_T);

                const Color Dull = {255,255,255,64};
                Sprite_t* spr = get_sprite(&scene->engine->assets, "upg_noti");
                if (p_pstate->collected >= data->min_cost)
                {
                    draw_sprite_scaled(
                        spr, 2,
                        (Vector2){text_pos.x + 60, text_pos.y + 92},
                        0, 2, GREEN
                    );
                    if (p_pstate->prev_collected < data->min_cost)
                    {
                        play_sfx(scene->engine, PLAYER_READY_SFX, false);
                    }
                }
                else
                {
                    draw_sprite_scaled(
                        spr, 2,
                        (Vector2){text_pos.x + 60, text_pos.y + 92},
                        0, 2, Dull
                    );
                }

                if (p_pstate->collected >= data->store.full_heal.cost)
                {
                    draw_sprite_scaled(
                        spr, 1,
                        (Vector2){text_pos.x + 160, text_pos.y + 92},
                        0, 2, RED
                    );
                    if (p_pstate->prev_collected < data->store.full_heal.cost)
                    {
                        play_sfx(scene->engine, PLAYER_READY_SFX, false);
                    }
                }
                else
                {
                    draw_sprite_scaled(
                        spr, 1,
                        (Vector2){text_pos.x + 160, text_pos.y + 92},
                        0, 2, Dull
                    );
                }

                if (p_pstate->collected >= data->store.escape.cost)
                {
                    draw_sprite_scaled(
                        spr, 0,
                        (Vector2){text_pos.x + 260, text_pos.y + 92},
                        0, 2, WHITE
                    );
                    if (p_pstate->prev_collected < data->store.escape.cost)
                    {
                        play_sfx(scene->engine, PLAYER_READY_SFX, false);
                    }
                }
                else
                {
                    draw_sprite_scaled(
                        spr, 0,
                        (Vector2){text_pos.x + 260, text_pos.y + 92},
                        0, 2, Dull
                    );
                }
                break;
            }
        }

    EndTextureMode();
}

static void game_over_check(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    if (sc_map_size_64v(&scene->ent_manager.entities_map[PLAYER_ENT_TAG]) == 0 || data->game_state == GAME_ENDED)
    {
        LevelSceneData_t* data = &((LevelScene_t*)scene)->data;
        data->game_state = GAME_ENDED;
        Entity_t* p_ent;
        sc_map_foreach_value(&scene->ent_manager.entities, p_ent)
        {
            if (get_component(p_ent, CSPAWNER_T) != NULL)
            {
                remove_component(p_ent, CSPAWNER_T);
            }
            if (p_ent->m_tag == ENEMY_ENT_TAG)
            {
                CLifeTimer_t* p_life = get_component(p_ent, CLIFETIMER_T);
                p_life->current_life = 0;
            }
            if (p_ent->m_tag == PLAYER_ENT_TAG)
            {
                remove_entity(&scene->ent_manager, p_ent->m_id);
            }

            CEmitter_t* p_emitter = get_component(p_ent, CEMITTER_T);
            if (p_emitter != NULL && is_emitter_handle_alive(&scene->part_sys, p_emitter->handle))
            {
                stop_emitter_handle(&scene->part_sys, p_emitter->handle);
                unload_emitter_handle(&scene->part_sys, p_emitter->handle);
            }
        }
        update_entity_manager(&scene->ent_manager);
    }
}

static void screenshake_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);


    if (data->screenshake_time > 0.0f)
    {
        data->screenshake_time -= scene->delta_time;
        data->camera.cam.target.x = -5 + 10.0f * (float)rand() / (float)RAND_MAX;
        data->camera.cam.target.y = -5 + 10.0f * (float)rand() / (float)RAND_MAX;
    }

        // Mass-Spring damper update
    float x = - data->camera.cam.target.x;
    float v = data->camera.current_vel.x; 
    float F = data->camera.k * x - data->camera.c * v;
    const float dt = scene->delta_time;
    float a_dt = F * dt / data->camera.mass;
    data->camera.cam.target.x += (data->camera.current_vel.x + a_dt * 0.5) * dt;

    x = - data->camera.cam.target.y;
    v = data->camera.current_vel.y; 
    F = data->camera.k * x - data->camera.c * v;
    a_dt = F * dt / data->camera.mass;
    data->camera.cam.target.y += (data->camera.current_vel.x + a_dt * 0.5) * dt;
    data->camera.current_vel.y += a_dt;

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
    LevelSceneData_t* lvl_data = &(((LevelScene_t*)scene->parent_scene)->data);

    switch(action)
    {
        case ACTION_SHOOT:
            if (!pressed && lvl_data->game_state == GAME_PLAYING)
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
                                    switch (i)
                                    {
                                        case 0:
                                        case 1:
                                        //case 2:
                                            p_weapons->modifier[i]++;
                                        break;
                                        case 2:
                                            p_life->current_life += HEALTH_INCREMENT;
                                            p_life->max_life += HEALTH_INCREMENT;
                                        break;
                                        case 3:
                                            if (p_life->current_life == p_life->max_life)
                                            {
                                                purchased = false;
                                            }
                                            else
                                            {
                                                p_life->current_life = p_life->max_life;
                                            }
                                        break;
                                        case 4:
                                        {
                                            // Game ending stuff
                                            LevelSceneData_t* lvl_data = &((LevelScene_t*)(scene->parent_scene))->data;
                                            create_final_enemy(
                                                &scene->parent_scene->ent_manager, 48,
                                                (Vector2){
                                                    lvl_data->game_field_size.x/2,
                                                    lvl_data->game_field_size.y/2
                                                }
                                            );
                                            CSpawner_t* p_spawner;
                                            unsigned int ent_idx;
                                            sc_map_foreach(&scene->parent_scene->ent_manager.component_map[CSPAWNER_T], ent_idx, p_spawner)
                                            {

                                                if (p_spawner->data.rank < MAX_RANK - 2)
                                                {
                                                    set_spawn_level(&p_spawner->data, MAX_RANK - 2);
                                                }
                                            }
                                            play_sfx(scene->engine, RANKUP_SFX, false);
                                        }
                                        break;
                                        case 5:
                                            p_weapons->unlocked[1] = true;
                                        break;
                                        case 6:
                                            p_weapons->unlocked[2] = true;
                                        break;
                                        case 7:
                                            p_weapons->unlocked[3] = true;
                                        break;
                                        default:
                                        break;
                                    }
                                }

                                if (purchased)
                                {
                                    play_sfx(scene->engine, BUYING_SFX, true);
                                    p_pstate->collected -= data->ui.upgrades[i].item->cost;
                                    data->curr_money = p_pstate->collected;
                                    data->ui.upgrades[i].item->cost +=  data->ui.upgrades[i].item->cost_increment;
                                    if (data->ui.upgrades[i].item->cost > data->ui.upgrades[i].item->cost_cap)
                                    {
                                        data->ui.upgrades[i].item->cost =  data->ui.upgrades[i].item->cost_cap;
                                    }

                                    // Min cost update, exclude heal and escape
                                    data->min_cost = 9999;
                                    for (uint8_t i = 0; i < N_UPGRADES; ++i)
                                    {
                                        if (data->ui.upgrades[i].item == &data->store.full_heal) continue;
                                        if (data->ui.upgrades[i].item == &data->store.escape) continue;
                                        if (data->ui.upgrades[i].item->remaining == 0) continue;
                                        
                                        if (data->ui.upgrades[i].item->cost < data->min_cost)
                                        {
                                            data->min_cost = data->ui.upgrades[i].item->cost;
                                        }
                                    }
                                }
                                else
                                {
                                    play_sfx(scene->engine, NOPE_SFX, true);
                                }

                            }
                            else
                            {
                                play_sfx(scene->engine, NOPE_SFX, true);
                            }
                            break;
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
                //play_sfx(scene->engine, PAUSE_SFX, true);
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

#define CONTAINER_OF(ptr, type, member) ({         \
    const typeof( ((type *)0)->member ) *__mptr = (ptr); \
    (type *)( (char *)__mptr - offsetof(type,member) );})

static void generate_shop_UI(ShopSceneData* data)
{
    Scene_t* scene = (Scene_t*)CONTAINER_OF(data, ShopScene_t, data);
    const int padding = 5;
    const int DOT_SPACING = 40;
    const int DESCRPTION_BOX_HEIGHT = 50;
    const int width = data->shop_rec.width - padding * 2;
    const int height = data->shop_rec.height - padding * 2;
    const int upgrade_height = (height - DESCRPTION_BOX_HEIGHT) / 7;

    Vector2 pos = {padding,padding};
    // Stats Upgrades: Hardcoded to 3 upgrades lol
    for (uint8_t i = 0; i < 3; ++i)
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

    data->ui.upgrades[3].button.box.x = padding;
    data->ui.upgrades[3].button.box.y = pos.y;
    data->ui.upgrades[3].button.box.width = data->ui.icon_size;
    data->ui.upgrades[3].button.box.height = data->ui.icon_size;
    data->ui.upgrades[3].dot_spacing = DOT_SPACING;
    data->ui.upgrades[3].show_dots = false;
    data->ui.upgrades[3].button.enabled = true;

    data->ui.upgrades[4].button.box.x = (width >> 1);
    data->ui.upgrades[4].button.box.y = pos.y;
    data->ui.upgrades[4].button.box.width = data->ui.icon_size;
    data->ui.upgrades[4].button.box.height = data->ui.icon_size;
    data->ui.upgrades[4].dot_spacing = DOT_SPACING;
    data->ui.upgrades[4].show_dots = false;
    data->ui.upgrades[4].button.enabled = true;

    pos.y += (upgrade_height > data->ui.icon_size) ? upgrade_height : data->ui.icon_size; 

    data->ui.upgrades[5].button.box.x = padding;
    data->ui.upgrades[5].button.box.y = pos.y;
    data->ui.upgrades[5].button.box.width = data->ui.icon_size;
    data->ui.upgrades[5].button.box.height = data->ui.icon_size;
    data->ui.upgrades[5].dot_spacing = DOT_SPACING;
    data->ui.upgrades[5].show_dots = false;
    data->ui.upgrades[5].button.enabled = true;

    data->ui.upgrades[6].button.box.x = (width >> 1);
    data->ui.upgrades[6].button.box.y = pos.y;
    data->ui.upgrades[6].button.box.width = data->ui.icon_size;
    data->ui.upgrades[6].button.box.height = data->ui.icon_size;
    data->ui.upgrades[6].dot_spacing = DOT_SPACING;
    data->ui.upgrades[6].show_dots = false;
    data->ui.upgrades[6].button.enabled = true;

    pos.y += (upgrade_height > data->ui.icon_size) ? upgrade_height : data->ui.icon_size; 

    data->ui.upgrades[7].button.box.x = (padding + (width >> 1)) >> 1;
    data->ui.upgrades[7].button.box.y = pos.y;
    data->ui.upgrades[7].button.box.width = data->ui.icon_size;
    data->ui.upgrades[7].button.box.height = data->ui.icon_size;
    data->ui.upgrades[7].dot_spacing = DOT_SPACING;
    data->ui.upgrades[7].show_dots = false;
    data->ui.upgrades[7].button.enabled = true;

    data->ui.upgrades[0].item = &data->store.firerate_upgrade;
    data->ui.upgrades[0].spr = get_sprite(&scene->engine->assets, "upg1_icon");
    data->ui.upgrades[1].item = &data->store.projspeed_upgrade;
    data->ui.upgrades[1].spr = get_sprite(&scene->engine->assets, "upg2_icon");
    //data->ui.upgrades[2].item = &data->store.damage_upgrade;
    //data->ui.upgrades[2].spr = get_sprite(&scene->engine->assets, "upg3_icon");
    data->ui.upgrades[2].item = &data->store.health_upgrade;
    data->ui.upgrades[2].spr = get_sprite(&scene->engine->assets, "upg4_icon");
    data->ui.upgrades[3].item = &data->store.full_heal;
    data->ui.upgrades[3].spr = get_sprite(&scene->engine->assets, "upg5_icon");
    data->ui.upgrades[4].item = &data->store.escape;
    data->ui.upgrades[4].spr = get_sprite(&scene->engine->assets, "upg6_icon");
    data->ui.upgrades[5].item = &data->store.thumper;
    data->ui.upgrades[5].spr = get_sprite(&scene->engine->assets, "upg7_icon");
    data->ui.upgrades[6].item = &data->store.maws;
    data->ui.upgrades[6].spr = get_sprite(&scene->engine->assets, "upg8_icon");

    data->ui.upgrades[7].item = &data->store.flux;
    data->ui.upgrades[7].spr = get_sprite(&scene->engine->assets, "upg9_icon");

    pos.y += data->ui.icon_size + padding; 
    data->ui.desc_box.x = padding;
    data->ui.desc_box.y = pos.y;
    data->ui.desc_box.width = width;
    data->ui.desc_box.height = DESCRPTION_BOX_HEIGHT * 2;
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

    create_start_target(
        &scene->scene.ent_manager, 64,
        (Vector2){
            scene->data.game_field_size.x / 2,
            scene->data.game_field_size.y / 4
        }
    );

    update_entity_manager(&scene->scene.ent_manager);

    ShopScene_t* shop_scene = (ShopScene_t*)scene->scene.subscene;
    shop_scene->data.store = (UpgradeStoreInventory){
        .firerate_upgrade = {200, 4, 4, 125, 1000},
        .projspeed_upgrade = {150, 4, 4, 150, 1000},
        //.damage_upgrade = {150, 3, 3, 150, 1000},
        .health_upgrade = {200, 4, 4, 150, 800},
        .full_heal = {100, -1, -1, 100, 10000},
        .escape = {1850, 1, 1, 0, 5000},
        .thumper = {300, 1, 1, 0, 1000},
        .maws = {500, 1, 1, 0, 1000},
        .flux = {400, 1, 1, 0, 2000},
    };
    shop_scene->data.min_cost = 150;
    shop_scene->data.curr_money = 0;

    for (uint8_t i = 0; i < N_UPGRADES; ++i)
    {
        shop_scene->data.ui.upgrades[i].button.enabled = true;
    }
    for (uint8_t i = 0; i < N_CIRCS; ++i)
    {
        circs[i].phase = 2 * PI * (float)rand()/ (float)RAND_MAX;
        circs[i].R = 25;
        circs[i].rate = 0.1f + 0.6f * (float)rand() / (float)RAND_MAX;
    }
    memset(&scene->data.survival_timer, 0, sizeof(scene->data.survival_timer));
    scene->data.game_state = GAME_STARTING;
    scene->data.endeffect_timer = 4000;
    scene->data.win_flag = false;
}

void init_level_scene(LevelScene_t* scene)
{
    init_scene(&scene->scene, &level_scene_render_func, &level_do_action);
    init_entity_tag_map(&scene->scene.ent_manager, PLAYER_ENT_TAG, 4);
    init_entity_tag_map(&scene->scene.ent_manager, BULLET_ENT_TAG, 256);

    scene->data.game_field_size = (Vector2){ARENA_WIDTH, ARENA_HEIGHT};
    scene->data.game_viewport = LoadRenderTexture(ARENA_WIDTH, ARENA_HEIGHT);
    scene->data.game_rec = (Rectangle){ARENA_START_X, ARENA_START_Y, ARENA_WIDTH, ARENA_HEIGHT};
    scene->scene.time_scale = 1.0f;

    memset(&scene->data.camera, 0, sizeof(LevelCamera_t));
    scene->data.camera.cam.rotation = 0.0f;
    scene->data.camera.cam.zoom = 1.0f;
    scene->data.camera.mass = 0.2f;
    scene->data.camera.k = 6.2f;
    scene->data.camera.c = 2.2f;

    sc_array_add(&scene->scene.systems, &update_player_emitter_system);
    sc_array_add(&scene->scene.systems, &weapon_cooldown_system);
    sc_array_add(&scene->scene.systems, &player_movement_input_system);
    sc_array_add(&scene->scene.systems, &magnet_update_system);
    sc_array_add(&scene->scene.systems, &global_external_forces_system);
    sc_array_add(&scene->scene.systems, &movement_update_system);

    sc_array_add(&scene->scene.systems, &screen_edge_check_system);
    sc_array_add(&scene->scene.systems, &invuln_update_system);
    sc_array_add(&scene->scene.systems, &money_collection_system);
    sc_array_add(&scene->scene.systems, &hitbox_update_system);

    // Entities may be 'removed' here
    sc_array_add(&scene->scene.systems, &life_update_system);

    // Entities may be dead after here
    // Systems at this point cannot further destroy more entities
    sc_array_add(&scene->scene.systems, &stop_emitter_on_death_system);
    sc_array_add(&scene->scene.systems, &spawner_update_system);
    //sc_array_add(&scene->scene.systems, &homing_update_system);
    sc_array_add(&scene->scene.systems, &spawned_update_system);
    sc_array_add(&scene->scene.systems, &ai_update_system);
    sc_array_add(&scene->scene.systems, &container_destroy_system);
    sc_array_add(&scene->scene.systems, &screenshake_update_system);
    sc_array_add(&scene->scene.systems, &sprite_animation_system);
    sc_array_add(&scene->scene.systems, &game_over_check);
    sc_array_add(&scene->scene.systems, &arena_render_func);

    sc_map_put_64(&scene->scene.action_map, KEY_A, ACTION_SELECT1);
    sc_map_put_64(&scene->scene.action_map, KEY_S, ACTION_SELECT2);
    sc_map_put_64(&scene->scene.action_map, KEY_D, ACTION_SELECT3);
    sc_map_put_64(&scene->scene.action_map, KEY_F, ACTION_SELECT4);
    sc_map_put_64(&scene->scene.action_map, KEY_J, ACTION_SELECT1);
    sc_map_put_64(&scene->scene.action_map, KEY_K, ACTION_SELECT2);
    sc_map_put_64(&scene->scene.action_map, KEY_L, ACTION_SELECT3);
    sc_map_put_64(&scene->scene.action_map, KEY_SEMICOLON, ACTION_SELECT4);
    sc_map_put_64(&scene->scene.action_map, KEY_SPACE, ACTION_BOOST);
    sc_map_put_64(&scene->scene.action_map, MOUSE_BUTTON_RIGHT, ACTION_MOVE);
    sc_map_put_64(&scene->scene.action_map, MOUSE_BUTTON_LEFT, ACTION_SHOOT);
    sc_map_put_64(&scene->scene.action_map, KEY_P, ACTION_PAUSE);
    sc_map_put_64(&scene->scene.action_map, KEY_Q, ACTION_PAUSE);
    sc_map_put_64(&scene->scene.action_map, KEY_Y, ACTION_RESTART);

    scene->scene.subscene->type = SUB_SCENE;
    init_scene(scene->scene.subscene, NULL, &shop_do_action);
#define STATS_HEIGHT 160
    scene->scene.subscene->parent_scene = &scene->scene;
    scene->scene.subscene->subscene = NULL;
    scene->scene.time_scale = 1.0f;
    scene->scene.subscene_pos = (Vector2){ARENA_START_X + ARENA_WIDTH + 20, ARENA_START_Y + STATS_Y_OFFSET};
    sc_array_add(&scene->scene.subscene->systems, &shop_check_mouse);
    sc_array_add(&scene->scene.subscene->systems, &shop_render_func);

    ShopScene_t* shop_scene = (ShopScene_t*)scene->scene.subscene;
    shop_scene->data.shop_viewport = LoadRenderTexture(350, ARENA_HEIGHT - 200);
    shop_scene->data.shop_rec = (Rectangle){0, 0, 350, ARENA_HEIGHT - 200};

    shop_scene->data.ui.icon_size = 70;
    shop_scene->data.ui.dot_size = 10;
    shop_scene->data.ui.pos = (Vector2){50, 50};

    generate_shop_UI(&shop_scene->data);

    scene->data.stat_view = LoadRenderTexture(
        total_icons_width, 180
    );
    restart_level_scene(scene);

    scene->data.credits_view = LoadRenderTexture(350, STATS_HEIGHT);
    BeginTextureMode(scene->data.credits_view);
        ClearBackground(BG_COLOUR);
        DrawText("Game made with raylib", 0, 0, 18, WHITE);
        DrawText("Art by me, made with Krita", 0, 23, 18, WHITE);
        DrawText("SFX by me, made with rfxgen", 0, 46, 18, WHITE);
        DrawText("Underwater Ambient Loop from Pixabay", 0, 69, 18, WHITE);
        DrawText("Audio Edited with Tenacity", 0, 92, 18, WHITE);
    EndTextureMode();
}

void free_level_scene(LevelScene_t* scene)
{
    UnloadRenderTexture(scene->data.game_viewport); // Unload render texture
    UnloadRenderTexture(scene->data.stat_view);
    UnloadRenderTexture(scene->data.credits_view);

    ShopScene_t* shop_scene = (ShopScene_t*)scene->scene.subscene;
    UnloadRenderTexture(shop_scene->data.shop_viewport); // Unload render texture
    free_scene(&scene->scene);
    free_scene(scene->scene.subscene);
}
