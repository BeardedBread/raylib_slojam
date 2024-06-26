#include "game_systems.h"
#include "EC.h"
#include "constants.h"
#include "assets_tag.h"

#include "ai_functions.h"
#include "ent_impl.h"
#include "level_ent.h"
#include "raymath.h"

static void simple_particle_system_update(Particle_t* part, void* user_data);

static inline unsigned long find_closest_entity(Scene_t* scene, Vector2 pos, unsigned int target_tag)
{
    Entity_t* p_target;
    float shortest_dist = 1e6;
    unsigned long target_idx = MAX_ENTITIES;
    sc_map_foreach_value(&scene->ent_manager.entities, p_target)
    {
        if (!p_target->m_alive) continue;
        if (p_target->m_tag != target_tag) continue;

        float curr_dist = Vector2DistanceSqr(p_target->position, pos);
        if (curr_dist < shortest_dist)
        {
            shortest_dist = curr_dist;
            target_idx = p_target->m_id;
        }
    }
    return target_idx;
}

void spawner_update_system(Scene_t* scene)
{
    CSpawner_t* p_spawner;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CSPAWNER_T], ent_idx, p_spawner)
    {
        Entity_t* p_ent = get_entity(&scene->ent_manager, ent_idx);
        p_spawner->spawner_main_logic(p_ent, &p_spawner->data, scene);
    }
}

void spawned_update_system(Scene_t* scene)
{
    CSpawn_t* p_spwn;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CSPAWNED_T], ent_idx, p_spwn)
    {
        Entity_t* p_ent = get_entity(&scene->ent_manager, ent_idx);
        if (p_ent != NULL && !p_ent->m_alive)
        {
            CSpawner_t* p_spawner = get_component(p_spwn->spawner, CSPAWNER_T);
            if (p_spawner != NULL)
            {
                p_spawner->spawnee_despawn_logic(p_ent, &p_spawner->data, scene);
            }
        }
    }
}

void money_collection_system(Scene_t* scene)
{
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        p_pstate->prev_collected = p_pstate->collected;
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        unsigned int money_idx;
        CMoney_t* p_money;
        sc_map_foreach(&scene->ent_manager.component_map[CMONEY_T], money_idx, p_money)
        {
            Entity_t* money_ent =  get_entity(&scene->ent_manager, money_idx);
            if (!money_ent->m_alive) continue;

            float dist = Vector2DistanceSqr(p_ent->position, money_ent->position);
            float range = p_ent->size + money_ent->size;
            if (dist < range * range)
            {
                p_pstate->collected += p_money->value;
                CLifeTimer_t* money_life = get_component(money_ent, CLIFETIMER_T);
                if (money_life != NULL)
                {
                    money_life->current_life = 0;
                }
                set_sfx_pitch(scene->engine, p_money->collect_sound, 0.8f + 0.4f * rand() / (float)RAND_MAX);
                play_sfx(scene->engine, p_money->collect_sound, true);
            }
        }
    }
}

void magnet_update_system(Scene_t* scene)
{
    CMagnet_t* p_magnet;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CMAGNET_T], ent_idx, p_magnet)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        unsigned int target_idx;
        CAttract_t* p_attract;
        sc_map_foreach(&scene->ent_manager.component_map[CATTRACTOR_T], target_idx, p_attract)
        {
            Entity_t* target_ent =  get_entity(&scene->ent_manager, target_idx);
            if (!p_ent->m_alive) continue;

            CTransform_t* p_ct = get_component(target_ent, CTRANSFORM_T);
            if (p_ct == NULL) continue;

            Vector2 vec = Vector2Subtract(p_ent->position, target_ent->position);
            if (
                Vector2LengthSqr(vec) < p_magnet->l2_range * p_attract->range_factor
                && (p_magnet->attract_idx & p_attract->attract_idx)
            )
            {
                Vector2 dir = Vector2Normalize(vec);
                p_ct->accel = Vector2Scale(dir, p_magnet->accel*p_attract->attract_factor);
            }

        }
    }
}

void life_update_system(Scene_t* scene)
{
    CLifeTimer_t* p_life;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CLIFETIMER_T], ent_idx, p_life)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (p_life->current_life <= 0)
        {
            CWallet_t* p_wallet = get_component(p_ent, CWALLET_T);
            //if (p_ent->m_tag == ENEMY_ENT_TAG)
            if (p_wallet != NULL)
            {
                Entity_t* money_ent = create_collectible(&scene->ent_manager, 6, p_wallet->value);
                if (money_ent != NULL)
                {
                    money_ent->position = p_ent->position;
                }
                ParticleEmitter_t emitter = {
                    .spr = NULL,
                    .config = get_emitter_conf(&scene->engine->assets, "part_ded"),
                    .position = {
                        .x = p_ent->position.x,
                        .y = p_ent->position.y,
                    },
                    .angle_offset = 0.0f,
                    .part_type = PARTICLE_LINE,
                    .n_particles = 10,
                    .user_data = scene,
                    .update_func = &simple_particle_system_update,
                    .emitter_update_func = NULL,
                };
                play_particle_emitter(&scene->part_sys, &emitter);
                play_sfx(scene->engine, ENEMY_DEAD_SFX, true);
            }
            else if (p_ent->m_tag == PLAYER_ENT_TAG)
            {
                play_sfx(scene->engine, PLAYER_DEATH_SFX, false);
            }
            remove_entity(&scene->ent_manager, ent_idx);
        }
        else 
        {
            p_life->poison += p_life->poison_value * scene->delta_time;
            if (p_life->poison > 100.0f)
            {
                // Poison is insta-kill
                p_life->current_life = 0;
                p_life->poison = 0.0f;
            }
        }
    }
}

void weapon_cooldown_system(Scene_t* scene)
{
    CWeapon_t* p_weapon ;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CWEAPON_T], ent_idx, p_weapon)
    {
        if (p_weapon->cooldown_timer > 0)
        {
            p_weapon->cooldown_timer -= scene->delta_time;
        }
    }

    CWeaponStore_t* p_weaponstore;
    sc_map_foreach(&scene->ent_manager.component_map[CWEAPONSTORE_T], ent_idx, p_weaponstore)
    {
        for (uint8_t i = 0; i < p_weaponstore->n_weapons; ++i)
        {
            if (!p_weaponstore->weapons[i].selected && p_weaponstore->weapons[i].cooldown_timer> 0)
            {
                p_weaponstore->weapons[i].cooldown_timer -= scene->delta_time / 5.0f;
            }
        }
    }
}

static inline void update_bullet(Entity_t* p_bullet, CWeapon_t* p_weapon, float angle, float speed)
{
    if (p_weapon->special_prop & 0x4)
    {
        float real_spread_range = p_weapon->spread_range * (1 - p_weapon->hold_timer * speed/ 150.0f);
        if (real_spread_range < 0.0f) real_spread_range = 0.0f;
        angle += -real_spread_range + 2*real_spread_range * (float)rand() / (float)RAND_MAX;
    }

    CTransform_t* bullet_ct = get_component(p_bullet, CTRANSFORM_T);
    bullet_ct->velocity = (Vector2){
        speed * cosf(angle),
        speed * sinf(angle),
    };
    bullet_ct->velocity_cap = speed * 2;
    CHitBoxes_t* bullet_hitbox = get_component(p_bullet, CHITBOXES_T);
    bullet_hitbox->atk = p_weapon->base_dmg * (1 + p_weapon->modifiers[2] * 0.2);
    bullet_hitbox->src = DMGSRC_PLAYER;
    bullet_hitbox->hit_sound = WEAPON1_HIT_SFX + p_weapon->weapon_idx;
    bullet_hitbox->knockback = p_weapon->bullet_kb;
    bullet_hitbox->dir = Vector2Normalize(bullet_ct->velocity);

    CLifeTimer_t* bullet_life = get_component(p_bullet, CLIFETIMER_T);
    bullet_life->poison_value = p_weapon->bullet_lifetime;

    if (p_weapon->special_prop & 0x1)
    {
        //bullet_life->poison_value = 0;
        bullet_ct->shape_factor = 2.3f + 0.3f * p_weapon->modifiers[1];
    }

    if (p_weapon->special_prop & 0x2)
    {
        bullet_hitbox->type = HITBOX_RAY;
        bullet_life->max_life = 4096;
        bullet_life->current_life = 4096;
        //bullet_life->poison_value = 0;
        remove_component(p_bullet, CSPRITE_T);
    }
    else
    {
        CSprite_t* p_cspr = get_component(p_bullet, CSPRITE_T);
        p_cspr->current_idx = p_weapon->weapon_idx;
        p_cspr->rotation = angle * 180 / PI;
    }
}

void player_movement_input_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;

    Vector2 raw_mouse_pos = Vector2Subtract(scene->mouse_pos, (Vector2){data->game_rec.x, data->game_rec.y});

    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_player = get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_T);
        
        //p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL);

        // Mouse aim direction
        if (p_player->m_tag == PLAYER_ENT_TAG)
        {
            Vector2 dir_vec = Vector2Subtract(raw_mouse_pos, p_player->position);
            p_pstate->aim_dir = Vector2Normalize(dir_vec);
            CSprite_t* p_cspr = get_component(p_player, CSPRITE_T);
            if (p_cspr != NULL)
            {
                p_cspr->rotation = atan2f(dir_vec.y, dir_vec.x) * 180 / PI;
            }
        }

        if (p_pstate->boost_cooldown > 0)
        {
            p_pstate->boost_cooldown -= scene->delta_time;
            if (p_pstate->boost_cooldown <= 0)
            {
                play_sfx(scene->engine, PLAYER_READY_SFX, false);
            }
        }

        if (p_pstate->boosting == 0b01)
        {
            if (p_pstate->boost_cooldown <= 0)
            {
                float boost_speed = Vector2Length(p_ctransform->velocity);
                p_ctransform->velocity = Vector2Scale(
                    p_pstate->aim_dir,
                    boost_speed > MIN_BOOST_SPEED ? boost_speed : MIN_BOOST_SPEED
                );
                p_pstate->boost_cooldown = BOOST_COOLDOWN;

                ParticleEmitter_t emitter = {
                    .spr = get_sprite(&scene->engine->assets, "ellip"),
                    .config = get_emitter_conf(&scene->engine->assets, "part_hit"),
                    .position = {
                        .x = p_player->position.x,
                        .y = p_player->position.y,
                    },
                    .angle_offset = atan2f(p_pstate->aim_dir.y, p_pstate->aim_dir.x) * 180 / PI - 180,
                    .part_type = PARTICLE_SPRITE,
                    .n_particles = 3,
                    .user_data = scene,
                    .update_func = &simple_particle_system_update,
                    .emitter_update_func = NULL,
                };
                play_particle_emitter(&scene->part_sys, &emitter);
                play_sfx(scene->engine, PLAYER_BOOST_SFX, false);
            }
            else
            {
                p_pstate->boosting &= ~(0b01);
            }
        }
        p_pstate->boosting <<= 1;
        p_pstate->boosting &= 0b11;


        if (p_pstate->moving & 1)
        {
            p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->aim_dir), MOVE_ACCEL);

            if (
                (p_ctransform->x_crossed > 0 && p_player->position.x < raw_mouse_pos.x)
                || (p_ctransform->x_crossed < 0 && p_player->position.x > raw_mouse_pos.x)
            )
            {
                p_ctransform->accel.x *= -1;
            }
            else
            {
                p_ctransform->x_crossed = 0;
            }

            if (
                (p_ctransform->y_crossed > 0 && p_player->position.y < raw_mouse_pos.y)
                || (p_ctransform->y_crossed < 0 && p_player->position.y > raw_mouse_pos.y)
            )
            {
                p_ctransform->accel.y *= -1;
            }
            else
            {
                p_ctransform->y_crossed = 0;
            }

            play_sfx(scene->engine, PLAYER_MOVE_SFX, false);
        }
        else
        {
            p_ctransform->x_crossed = 0;
            p_ctransform->y_crossed = 0;
            stop_sfx(scene->engine, PLAYER_MOVE_SFX);
        }
        p_pstate->moving <<= 1;
        p_pstate->moving &= 0b11;

        // Shoot
        if (p_pstate->shoot > 0)
        {
            CWeapon_t* p_weapon = get_component(p_player, CWEAPON_T);
            if (p_weapon == NULL) goto shoot_end;
            if (p_weapon->cooldown_timer > 0) goto shoot_end;
            if (p_weapon->special_prop & 0x2)
            {
                if (p_weapon->hold_timer < 3.0f)
                {
                    p_weapon->hold_timer += scene->delta_time;
                }
                if (p_pstate->shoot != 0b10) goto shoot_end;
            }

            float speed = p_weapon->proj_speed * (1 + 0.1f * p_weapon->modifiers[1]);
            if (p_weapon != NULL && p_weapon->cooldown_timer <= 0)
            {
                if (p_weapon->special_prop & 0x2)
                {
                    data->screenshake_time = 0.1f;
                }
                uint8_t bullets = p_weapon->n_bullets;
                float angle = atan2f(p_pstate->aim_dir.y, p_pstate->aim_dir.x);
                float angle_increment = 0.0f;
                if (p_weapon->n_bullets % 2 == 1)
                {
                    Entity_t* p_bullet = create_bullet(&scene->ent_manager);
                    p_bullet->position = p_player->position;

                    update_bullet(p_bullet, p_weapon, angle, speed);

                    if (p_weapon->special_prop & 0x1)
                    {
                        unsigned long target_idx = find_closest_entity(scene, raw_mouse_pos, ENEMY_ENT_TAG);

                        CAIFunction_t* p_ai = add_component(p_bullet, CAIFUNC_T);
                        p_ai->target_idx = target_idx;
                        p_ai->target_tag = ENEMY_ENT_TAG;
                        p_ai->accl = 1500 + p_weapon->modifiers[1] * 350;
                        p_ai->func = &homing_target_func;
                    }

                    bullets--;
                    angle -= p_weapon->spread_range;
                    angle_increment = p_weapon->spread_range;

                }
                else
                {
                    angle -= p_weapon->spread_range / 2;
                    angle_increment = p_weapon->spread_range / 2;
                }

                bullets >>= 1;

                for (uint8_t i = 0; i < bullets; i++)
                {
                    Entity_t* p_bullet = create_bullet(&scene->ent_manager);
                    p_bullet->position = p_player->position;
                    update_bullet(p_bullet, p_weapon, angle, speed);
                    if (p_weapon->special_prop & 0x1)
                    {
                        unsigned long target_idx = find_closest_entity(scene, raw_mouse_pos, ENEMY_ENT_TAG);
                        CAIFunction_t* p_ai = add_component(p_bullet, CAIFUNC_T);
                        p_ai->target_idx = target_idx;
                        p_ai->target_tag = ENEMY_ENT_TAG;
                        p_ai->accl = 1500 + p_weapon->modifiers[1] * 350;
                        p_ai->func = &homing_target_func;
                    }

                    p_bullet = create_bullet(&scene->ent_manager);
                    p_bullet->position = p_player->position;
                    update_bullet(p_bullet, p_weapon, angle + 2 * angle_increment, speed);
                    if (p_weapon->special_prop & 0x1)
                    {
                        unsigned long target_idx = find_closest_entity(scene, raw_mouse_pos, ENEMY_ENT_TAG);
                        CAIFunction_t* p_ai = add_component(p_bullet, CAIFUNC_T);
                        p_ai->target_idx = target_idx;
                        p_ai->target_tag = ENEMY_ENT_TAG;
                        p_ai->accl = 1500 + p_weapon->modifiers[1] * 350;
                        p_ai->func = &homing_target_func;
                    }

                    angle -= p_weapon->spread_range;
                    angle_increment += p_weapon->spread_range;
                }

                p_weapon->cooldown_timer = 1.0f / (p_weapon->fire_rate  * (1 + p_weapon->modifiers[0] * 0.08));
                play_sfx(scene->engine, WEAPON1_FIRE_SFX + p_weapon->weapon_idx, true);
            }

            p_weapon->hold_timer = 0.0f;
        }
shoot_end:
        p_pstate->shoot <<= 1;
        p_pstate->shoot &= 0x3;
    }
}

void global_external_forces_system(Scene_t* scene)
{
    CTransform_t * p_ctransform;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CTRANSFORM_T], ent_idx, p_ctransform)
    {
        // Friction
        p_ctransform->accel = Vector2Add(
            p_ctransform->accel,
            Vector2Scale(p_ctransform->velocity, -0.5 * p_ctransform->shape_factor)
        );
    }
}

void movement_update_system(Scene_t* scene)
{
    float delta_time = scene->delta_time;
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
    // Update movement
    CTransform_t * p_ctransform;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CTRANSFORM_T], ent_idx, p_ctransform)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (!p_ctransform->active)
        {
            memset(&p_ctransform->velocity, 0, sizeof(Vector2));
            memset(&p_ctransform->accel, 0, sizeof(Vector2));
            continue;
        }
        p_ctransform->prev_position = p_ent->position;
        p_ctransform->prev_velocity = p_ctransform->velocity;

        p_ctransform->velocity = Vector2Add(
            p_ctransform->velocity,
            Vector2Scale(p_ctransform->accel, delta_time)
        );

        float vel = Vector2Length(p_ctransform->velocity);
        if ( vel > p_ctransform->velocity_cap)
        {
            p_ctransform->velocity = Vector2Scale(Vector2Normalize(p_ctransform->velocity), vel);
        }

        // 3 dp precision
        if (fabs(p_ctransform->velocity.x) < 1e-3) p_ctransform->velocity.x = 0;
        if (fabs(p_ctransform->velocity.y) < 1e-3) p_ctransform->velocity.y = 0;

        // Store previous position before update
        p_ctransform->prev_position = p_ent->position;
        p_ent->position = Vector2Add(
            p_ent->position,
            Vector2Scale(p_ctransform->velocity, delta_time)
        );
        memset(&p_ctransform->accel, 0, sizeof(p_ctransform->accel));

        // Absolute boundary
        if (p_ent->position.x < -180
         || (p_ent->position.x > data->game_field_size.x + 180)
        
         || (p_ent->position.y < -180)
         || (p_ent->position.y > data->game_field_size.y + 180)
         )
        {
            remove_entity(&scene->ent_manager, p_ent->m_id);
            continue;
        }

        // Level boundary collision
        if (p_ctransform->edge_b == EDGE_BOUNCE)
        {
            if (
                p_ent->position.x - p_ent->size < 0
                && p_ctransform->prev_position.x - p_ent->size >= 0
            )
            {
                p_ent->position.x =  p_ent->size;
                p_ctransform->velocity.x *= -1.0f;
            }
            else if (
                p_ent->position.x + p_ent->size > data->game_field_size.x
                && (p_ctransform->prev_position.x + p_ent->size <= data->game_field_size.x )
            )
            {
                p_ent->position.x =  data->game_field_size.x - p_ent->size;
                p_ctransform->velocity.x *= -1.0f;
            }
            
            if (
                p_ent->position.y - p_ent->size < 0
                && p_ctransform->prev_position.y - p_ent->size >= 0
            )
            {
                p_ent->position.y =  p_ent->size;
                p_ctransform->velocity.y *= -1.0f;
            }
            else if (
                p_ent->position.y + p_ent->size > data->game_field_size.y
                && p_ctransform->prev_position.y + p_ent->size <= data->game_field_size.y
            )
            {
                p_ent->position.y =  data->game_field_size.y - p_ent->size;
                p_ctransform->velocity.y *= -1.0f;
            }
        }
        else if (p_ctransform->edge_b == EDGE_WRAPAROUND)
        {
            if (p_ent->position.x < 0)
            {
                p_ent->position.x += data->game_field_size.x;
                p_ctransform->x_crossed--;
            }
            else if (p_ent->position.x > data->game_field_size.x)
            {
                p_ent->position.x -= data->game_field_size.x;
                p_ctransform->x_crossed++;
            }
            
            if (p_ent->position.y < 0)
            {
                p_ent->position.y += data->game_field_size.y;
                p_ctransform->y_crossed--;
            }
            else if (p_ent->position.y > data->game_field_size.y)
            {
                p_ent->position.y -= data->game_field_size.y;
                p_ctransform->y_crossed++;
            }
        }
        
    }
}
void invuln_update_system(Scene_t* scene)
{
    unsigned int ent_idx;
    CHurtbox_t* p_hurtbox;
    sc_map_foreach(&scene->ent_manager.component_map[CHURTBOX_T], ent_idx, p_hurtbox)
    {
        if (p_hurtbox->invuln_timer > 0)
        {
            p_hurtbox->invuln_timer -= scene->delta_time;
        }
    }
}

void screen_edge_check_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);

    unsigned int ent_idx;
    CTransform_t* p_ct;
    sc_map_foreach(&scene->ent_manager.component_map[CTRANSFORM_T], ent_idx, p_ct)
    {
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (p_ct->edge_b == EDGE_WRAPAROUND)
        {
            uint8_t flip_x = 1;
            uint8_t flip_y = 1;
            if (p_ent->position.x < p_ent->size)
            {
                flip_x = 2;
            }
            if (p_ent->position.x > data->game_field_size.x - p_ent->size)
            {
                flip_x = 0;
            }

            if (p_ent->position.y < p_ent->size)
            {
                flip_y = 2;
            }
            if (p_ent->position.y > data->game_field_size.y - p_ent->size)
            {
                flip_y = 0;
            }
            p_ct->boundary_checks = (flip_y << 2) | flip_x;
        }
    }
}

void hitbox_update_system(Scene_t* scene)
{
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);

    unsigned int ent_idx;
    CHitBoxes_t* p_hitbox;
    sc_map_foreach(&scene->ent_manager.component_map[CHITBOXES_T], ent_idx, p_hitbox)
    {
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive) continue;


        unsigned int other_ent_idx;
        CHurtbox_t* p_hurtbox;
        CLifeTimer_t* p_life = get_component(p_ent, CLIFETIMER_T);
        sc_map_foreach(&scene->ent_manager.component_map[CHURTBOX_T], other_ent_idx, p_hurtbox)
        {
            if (other_ent_idx == ent_idx) continue;
            if (p_hurtbox->invuln_timer > 0) continue;

            Entity_t* p_other_ent = get_entity(&scene->ent_manager, other_ent_idx);

            if (p_hitbox->src == p_hurtbox->src) continue;
            if (!p_other_ent->m_alive) continue;

            Vector2 target_positions[4];
            uint8_t n_pos = 1;
            target_positions[0] = p_other_ent->position;

            // Account for wraparound hitboxes
            CTransform_t* p_ct = get_component(p_other_ent, CTRANSFORM_T);
            if (p_ct != NULL && p_ct->edge_b == EDGE_WRAPAROUND)
            {
                
                int8_t flip_x = (int8_t)(p_ct->boundary_checks & 0b11) - 1;
                int8_t flip_y = (int8_t)((p_ct->boundary_checks >> 2) & 0b11) - 1;


                Vector2 clone_pos = p_other_ent->position;
                if (flip_x != 0 && flip_y != 0)
                {
                    clone_pos.x += flip_x * data->game_field_size.x;
                    target_positions[1] = clone_pos;
                    clone_pos.y += flip_y * data->game_field_size.y;
                    target_positions[2] = clone_pos;
                    clone_pos.x -= flip_x * data->game_field_size.x;
                    target_positions[3] = clone_pos;
                    n_pos = 4;
                }
                else if (flip_x != 0)
                {
                    clone_pos.x += flip_x * data->game_field_size.x;
                    target_positions[1] = clone_pos;
                    n_pos = 2;
                }
                else
                {
                    clone_pos.y += flip_y * data->game_field_size.y;
                    target_positions[1] = clone_pos;
                    n_pos = 2;
                }
            }

            for (uint8_t i = 0; i < n_pos; i++)
            {
                float dist = 0.0f;
                if (p_hitbox->type == HITBOX_RAY)
                {
                    Vector2 dir_norm = Vector2Normalize(p_hitbox->dir);
                    float t = Vector2DotProduct(
                        Vector2Subtract(p_other_ent->position, p_ent->position),
                        dir_norm
                    );
                    if (t < 0) t = 0;

                    Vector2 p = Vector2Add(p_ent->position, Vector2Scale(dir_norm, t));
                    dist = Vector2Distance(p, target_positions[i]);
                }
                else
                {
                    dist = Vector2Distance(p_ent->position, target_positions[i]);
                }

                // On hit
                if (dist < p_hitbox->size + p_hurtbox->size)
                {
                    Vector2 attack_dir = Vector2Subtract(target_positions[i], p_ent->position);
                    p_hurtbox->attack_dir = (Vector2){0,0};
                    p_hurtbox->invuln_timer = p_hurtbox->invuln_time;
                    CLifeTimer_t* p_other_life = get_component(p_other_ent, CLIFETIMER_T);

                    if (p_other_life != NULL)
                    {
                        int16_t prev_life = p_other_life->current_life;
                        if (p_other_life->current_life <= 0)
                        {
                            // Already dead, move on
                            break;
                        }
                        p_other_life->current_life -= p_hitbox->atk;
                        if (p_other_ent->m_tag == PLAYER_ENT_TAG)
                        {
                            // Last chance if previous health is not in critical health
                            // Just a little mercy for huge hits
                            if (prev_life > CRIT_HEALTH && p_other_life->current_life <= 0)
                            {
                                p_other_life->current_life = 1;
                            }
                        }
                        if (p_other_life->current_life <= 0)
                        {
                            p_hurtbox->attack_dir = attack_dir;
                        }
                    }
                    CTransform_t* p_ct = get_component(p_other_ent, CTRANSFORM_T);

                    if (p_ct != NULL)
                    {
                        float kb = 10 * p_hitbox->knockback * p_hurtbox->kb_mult;
                        if (kb > 400) kb = 400;

                        p_ct->velocity = 
                        Vector2Add(
                            p_ct->velocity,
                            Vector2Scale(
                                Vector2Normalize(attack_dir),
                                kb
                            )
                        );
                    }

                    ParticleEmitter_t emitter = {
                        .spr = NULL,
                        .config = get_emitter_conf(&scene->engine->assets, "part_hit"),
                        .position = {
                            .x = p_other_ent->position.x,
                            .y = p_other_ent->position.y,
                        },
                        .angle_offset = atan2f(attack_dir.y, attack_dir.x) * 180 / PI - 180,
                        .part_type = PARTICLE_SQUARE,
                        .n_particles = 5,
                        .user_data = scene,
                        .update_func = &simple_particle_system_update,
                        .emitter_update_func = NULL,
                    };
                    play_particle_emitter(&scene->part_sys, &emitter);
                    if (p_hitbox->hit_sound < N_SFX)
                    {
                        play_sfx(scene->engine, p_hitbox->hit_sound, true);
                    }

                    if (p_life != NULL)
                    {
                        p_life->current_life--;
                    }

                    if (p_other_ent->m_tag == PLAYER_ENT_TAG)
                    {
                        data->screenshake_time = 0.2f;

                        if (p_other_life->current_life <= CRIT_HEALTH && p_other_life->current_life > 0)
                        {
                            play_sfx(scene->engine, PLAYER_WARN_SFX, false);
                        }
                    }
                    break;
                }
            }

            if (p_life->current_life <= 0)
            {
                break;
            }
        }
    }
}

void container_destroy_system(Scene_t* scene)
{
    unsigned int ent_idx;
    CContainer_t* p_container;
    sc_map_foreach(&scene->ent_manager.component_map[CCONTAINER_T], ent_idx, p_container)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (p_ent->m_alive) continue;
        CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
        CHurtbox_t* p_hurtbox = get_component(p_ent, CHURTBOX_T);
        CSpawn_t* p_spwn = get_component(p_ent, CSPAWNED_T);
        float angle = 0.0f;
        if (p_ct != NULL)
        {
            angle = atan2f(p_ct->velocity.y, p_ct->velocity.x);
        }

        if (p_hurtbox != NULL)
        {
            if (p_hurtbox->attack_dir.y != 0 && p_hurtbox->attack_dir.x != 0)
            {
                angle = atan2f(p_hurtbox->attack_dir.y, p_hurtbox->attack_dir.x);
            }
        }

        switch (p_container->item)
        {
            case CONTAINER_ENEMY:
            {
                if (p_ent->size / SIZE_SPLIT_FACTOR <= ENEMY_MIN_SIZE || p_container->num == 0) break;
                CTransform_t* current_ct = get_component(p_ent, CTRANSFORM_T);
                float momentum = p_ent->size * Vector2Length(current_ct->velocity);
                momentum /= p_container->num;
                float new_sz = p_ent->size / SIZE_SPLIT_FACTOR;
                float speed = momentum / new_sz;

                float increment = 2 * PI / p_container->num;
                angle += increment / 2;
                for (uint8_t i = 0; i < p_container->num; ++i)
                {
                    int32_t value = 0;
                    CWallet_t* p_wallet = get_component(p_ent, CWALLET_T);
                    if (p_wallet != NULL)
                    {
                        value = (p_wallet->value >> 1) + 1;
                    }
                    Entity_t* p_enemy = create_enemy(&scene->ent_manager, new_sz, value);
                    p_enemy->position = p_ent->position;

                    CTransform_t* enemy_ct = get_component(p_enemy, CTRANSFORM_T);

                    speed += 50 * (float)rand() / (float)RAND_MAX;
                    enemy_ct->velocity = (Vector2){
                        speed * cosf(angle),
                        speed * sinf(angle),
                    };
                    angle += increment;
                    if (p_spwn != NULL)
                    {

                        CSpawner_t* p_spawner = get_component(p_spwn->spawner, CSPAWNER_T);
                        if (p_spawner != NULL)
                        {
                            p_spawner->child_spawn_logic(p_enemy, &p_spawner->data, scene);
                            CSpawn_t* new_spwn = add_component(p_enemy, CSPAWNED_T);
                            new_spwn->spawner = p_spwn->spawner;
                        }
                    }
                }
            }
            break;
            case CONTAINER_GOAL:
            {
                //LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
                Entity_t* p_goal = create_end_item(&scene->ent_manager, 32);
                p_goal->position = p_ent->position;
            }
            break;
            case CONTAINER_ENDING:
            {
                LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
                data->game_state = GAME_ENDED;
                data->endeffect_timer = 0;
                data->endeffect_pos = p_ent->position;
                data->win_flag = true;
            }
            break;
            case CONTAINER_START:
            {
                LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
                if (data->game_state == GAME_STARTING)
                {
                    create_spawner(&scene->ent_manager);
                    play_sfx(scene->engine, RANKUP_SFX, false);
                    data->game_state = GAME_PLAYING;
                    ParticleEmitter_t emitter = {
                        .spr = NULL,
                        .config = get_emitter_conf(&scene->engine->assets, "part_ded"),
                        .position = {
                            .x = p_ent->position.x,
                            .y = p_ent->position.y,
                        },
                        .angle_offset = 0.0f,
                        .part_type = PARTICLE_LINE,
                        .n_particles = 20,
                        .user_data = scene,
                        .update_func = &simple_particle_system_update,
                        .emitter_update_func = NULL,
                    };
                    play_particle_emitter(&scene->part_sys, &emitter);
                    play_sfx(scene->engine, ENEMY_DEAD_SFX, true);
                }
            }
            break;
            default:
            break;
        }
    }
}

void sprite_animation_system(Scene_t* scene)
{
    unsigned int ent_idx;
    CSprite_t* p_cspr;
    sc_map_foreach(&scene->ent_manager.component_map[CSPRITE_T], ent_idx, p_cspr)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        // Update animation state
        unsigned int next_idx = p_cspr->current_idx;
        if (p_cspr->transition_func != NULL)
        {
            next_idx = p_cspr->transition_func(p_ent);
        }
        if (p_cspr->pause) return;

        bool reset = p_cspr->current_idx != next_idx;
        p_cspr->current_idx = next_idx;

        Sprite_t* spr = p_cspr->sprites[p_cspr->current_idx];
        if (spr == NULL) continue;

        if (reset) p_cspr->current_frame = 0;

        // Animate it (handle frame count)
        p_cspr->elapsed++;
        if (p_cspr->elapsed == spr->speed)
        {
            p_cspr->current_frame++;
            p_cspr->current_frame %= spr->frame_count;
            p_cspr->elapsed = 0;
        }
    }
}

void ai_update_system(Scene_t* scene)
{
    unsigned int ent_idx;
    CAIFunction_t* p_enemyai;
    sc_map_foreach(&scene->ent_manager.component_map[CAIFUNC_T], ent_idx, p_enemyai)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive) continue;

        // Give 0.4s of activation time
        if (p_enemyai->lag_time > 0.0f)
        {
            p_enemyai->lag_time -= scene->delta_time;
            continue;
        }

        // Re-target
        Entity_t* p_target = get_entity(&scene->ent_manager, p_enemyai->target_idx);
        if (p_target == NULL || !p_target->m_alive)
        {
            p_enemyai->target_idx = find_closest_entity(scene, p_ent->position, p_enemyai->target_tag);
            continue;
        }

        if (p_enemyai->func != NULL)
        {
            p_enemyai->func(p_ent, scene);
        }
    }
}

void update_player_emitter_system(Scene_t* scene)
{
    CPlayerState_t* p_pstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CEmitter_t* p_emitter = get_component(p_ent, CEMITTER_T);

        if (p_pstate->moving & 1)
        {
            if (!is_emitter_handle_alive(&scene->part_sys, p_emitter->handle))
            {
                ParticleEmitter_t emitter = {
                    .spr = NULL,
                    .part_type = PARTICLE_SQUARE,
                    .config = get_emitter_conf(&scene->engine->assets, "part_fire"),
                    //.position = new_pos,
                    .position = p_ent->position,
                    .n_particles = 50,
                    .user_data = scene,
                    .update_func = &simple_particle_system_update,
                    .emitter_update_func = NULL,
                };
                p_emitter->handle = play_particle_emitter(&scene->part_sys, &emitter);
            }
            
            play_emitter_handle(&scene->part_sys, p_emitter->handle);
        }
        else
        {
            stop_emitter_handle(&scene->part_sys, p_emitter->handle);
            unload_emitter_handle(&scene->part_sys, p_emitter->handle);
        }
        update_emitter_handle_rotation(&scene->part_sys, p_emitter->handle, atan2f(p_pstate->aim_dir.y, p_pstate->aim_dir.x) *180/PI - 180);
    }
}

void stop_emitter_on_death_system(Scene_t* scene)
{
    CEmitter_t* p_emitter;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CEMITTER_T], ent_idx, p_emitter)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);

        if (!p_ent->m_alive)
        {
            if (is_emitter_handle_alive(&scene->part_sys, p_emitter->handle))
            {
                stop_emitter_handle(&scene->part_sys, p_emitter->handle);
                unload_emitter_handle(&scene->part_sys, p_emitter->handle);
            }
        }
        else
        {
            Vector2 new_pos = Vector2Add(p_ent->position, p_emitter->offset);
            update_emitter_handle_position(&scene->part_sys, p_emitter->handle, new_pos);
        }
    }
}

void simple_particle_system_update(Particle_t* part, void* user_data)
{
    Scene_t* scene = (Scene_t*)user_data;

    part->position = Vector2Add(
        part->position,
        Vector2Scale(part->velocity, scene->delta_time)
    );
}
