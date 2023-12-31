#include "game_systems.h"
#include "EC.h"
#include "constants.h"
#include "assets_tag.h"

#include "ent_impl.h"
#include "raymath.h"

static void simple_particle_system_update(Particle_t* part, void* user_data);

static inline unsigned long find_closest_entity(Scene_t* scene, Vector2 pos)
{
    Entity_t* p_target;
    float shortest_dist = 1e6;
    unsigned long target_idx = MAX_ENTITIES;
    sc_map_foreach_value(&scene->ent_manager.entities, p_target)
    {
        if (p_target->m_tag != ENEMY_ENT_TAG) continue;

        float curr_dist = Vector2DistanceSqr(p_target->position, pos);
        if (curr_dist < shortest_dist)
        {
            shortest_dist = curr_dist;
            target_idx = p_target->m_id;
        }
    }
    return target_idx;
}

void ai_update_system(Scene_t* scene)
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
            p_spawner->spawnee_despawn_logic(p_ent, &p_spawner->data, scene);
        }
    }
}

void homing_update_system(Scene_t* scene)
{
    CHoming_t* p_homing;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CHOMING_T], ent_idx, p_homing)
    {
        Entity_t* p_ent = get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive) continue;

        CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
        if (p_ct == NULL)
        {
            remove_component(p_ent, CHOMING_T);
            continue;
        }

        float vel = Vector2Length(p_ct->velocity);
        if (vel > p_ct->velocity_cap)
        {
            p_ct->velocity = Vector2Scale(Vector2Normalize(p_ct->velocity), vel);
        }

        Vector2 self_pos = p_ent->position;
        Vector2 self_vel = p_ct->velocity;

        Entity_t* p_target = get_entity(&scene->ent_manager, p_homing->target_idx);
        if (p_target == NULL || !p_target->m_alive)
        {
            // Re-target but don't update
            unsigned long target_idx = find_closest_entity(scene, self_pos);
            if (target_idx != MAX_ENTITIES)
            {
                p_homing->target_idx = target_idx;
            }
            p_ct->shape_factor = 0;
            continue;
        }
        p_ct->shape_factor = 7;

        Vector2 target_pos = p_target->position;
        Vector2 target_vel = {0,0};
        CTransform_t* target_ct = get_component(p_target, CTRANSFORM_T);
        if (p_ct != NULL)
        {
            target_vel = target_ct->velocity;
        }

        Vector2 v_s = Vector2Subtract(target_vel, self_vel);
        Vector2 to_target = Vector2Subtract(target_pos, self_pos);
        float v_c = Vector2DotProduct(
            Vector2Scale(v_s, -1.0f),
            Vector2Normalize(to_target)
        );

        const float rocket_accel = 4000.0f;
        float eta =
            -v_c / rocket_accel
            + sqrtf(
                v_c * v_c / (rocket_accel * rocket_accel)
                + 2 * Vector2Length(to_target) / rocket_accel
            )
        ;

        Vector2 predict_pos = Vector2Add(target_pos, Vector2Scale(target_vel, eta));

        Vector2 to_predict = Vector2Subtract(predict_pos, self_pos);
        p_ct->accel = Vector2Scale(
            Vector2Normalize(to_predict),
            rocket_accel
        );

        CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
        if (p_cspr != NULL)
        {
            float angle = atan2f(to_target.y, to_target.x);
            p_cspr->rotation = angle * 180 / PI;
    }
    }
}

void money_collection_system(Scene_t* scene)
{
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
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
                set_sfx_pitch(scene->engine, COLLECT_SFX, 0.8f + 0.4f * rand() / (float)RAND_MAX);
                play_sfx(scene->engine, COLLECT_SFX, true);
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
            if (p_ent->m_tag == ENEMY_ENT_TAG)
            {
                CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
                float value = 1.0; // always have 1
                value += (100.0f - p_ent->size) / 8 + Vector2LengthSqr(p_ct->velocity) / 250000.0;
                Entity_t* money_ent = create_collectible(&scene->ent_manager, 10, (int32_t)value);
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
            else
            {
            }
            remove_entity(&scene->ent_manager, ent_idx);
        }
        else
        {
            p_life->poison += p_life->poison_value * scene->delta_time;
            if (p_life->poison > 100.0f)
            {
                p_life->current_life--;
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
                p_pstate->boost_cooldown = 3.0f;

                ParticleEmitter_t emitter = {
                    .spr = NULL,
                    .config = get_emitter_conf(&scene->engine->assets, "part_hit"),
                    .position = {
                        .x = p_player->position.x,
                        .y = p_player->position.y,
                    },
                    .angle_offset = atan2f(p_pstate->aim_dir.y, p_pstate->aim_dir.x) * 180 / PI - 180,
                    .part_type = PARTICLE_LINE,
                    .n_particles = 10,
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
            
            play_sfx(scene->engine, PLAYER_MOVE_SFX, false);
        }
        else
        {
            stop_sfx(scene->engine, PLAYER_MOVE_SFX);
        }
        p_pstate->moving <<= 1;
        p_pstate->moving &= 0b11;

        // Shoot
        if (p_pstate->shoot > 0)
        {
            CWeapon_t* p_weapon = get_component(p_player, CWEAPON_T);
            float speed = p_weapon->proj_speed * (1 + 0.1f * p_weapon->modifiers[1]);
            if (p_weapon != NULL && p_weapon->cooldown_timer <= 0)
            {
                uint8_t bullets = p_weapon->n_bullets;
                float angle = atan2f(p_pstate->aim_dir.y, p_pstate->aim_dir.x);
                float angle_increment = 0.0f;
                if (p_weapon->n_bullets % 2 == 1)
                {
                    Entity_t* p_bullet = create_bullet(&scene->ent_manager);
                    CTransform_t* bullet_ct = get_component(p_bullet, CTRANSFORM_T);
                    bullet_ct->velocity = Vector2Scale(p_pstate->aim_dir, speed);
                    bullet_ct->velocity_cap = speed * 2;
                    p_bullet->position = p_player->position;
                    CHitBoxes_t* bullet_hitbox = get_component(p_bullet, CHITBOXES_T);
                    bullet_hitbox->atk = p_weapon->base_dmg * (1 + p_weapon->modifiers[2] * 0.2);
                    bullet_hitbox->src = DMGSRC_PLAYER;
                    bullet_hitbox->hit_sound = WEAPON1_HIT_SFX + p_weapon->weapon_idx;

                    CLifeTimer_t* bullet_life = get_component(p_bullet, CLIFETIMER_T);
                    bullet_life->poison_value = p_weapon->bullet_lifetime;

                    if (p_weapon->homing)
                    {
                        unsigned long target_idx = find_closest_entity(scene, raw_mouse_pos);
                        CHoming_t* p_homing = add_component(p_bullet, CHOMING_T);
                        p_homing->target_idx = target_idx;
                        bullet_life->poison_value = 0;
                        bullet_ct->shape_factor = 7 + 1.2f * p_weapon->modifiers[1];
                    }

                    CSprite_t* p_cspr = get_component(p_bullet, CSPRITE_T);
                    p_cspr->current_idx = p_weapon->weapon_idx;
                    p_cspr->rotation = angle * 180 / PI;

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
                    CTransform_t* bullet_ct = get_component(p_bullet, CTRANSFORM_T);
                    bullet_ct->velocity = (Vector2){
                        speed * cosf(angle),
                        speed * sinf(angle),
                    };
                    p_bullet->position = p_player->position;
                    CHitBoxes_t* bullet_hitbox = get_component(p_bullet, CHITBOXES_T);
                    bullet_hitbox->atk = p_weapon->base_dmg;
                    bullet_hitbox->src = DMGSRC_PLAYER;
                    bullet_hitbox->hit_sound = WEAPON1_HIT_SFX + p_weapon->weapon_idx;

                    CLifeTimer_t* bullet_life = get_component(p_bullet, CLIFETIMER_T);
                    bullet_life->poison_value = p_weapon->bullet_lifetime;

                    CSprite_t* p_cspr = get_component(p_bullet, CSPRITE_T);
                    p_cspr->current_idx = p_weapon->weapon_idx;
                    p_cspr->rotation = angle * 180 / PI;

                    p_bullet = create_bullet(&scene->ent_manager);
                    bullet_ct = get_component(p_bullet, CTRANSFORM_T);
                    bullet_ct->velocity = (Vector2){
                        speed * cosf(angle + 2 * angle_increment),
                        speed * sinf(angle + 2 * angle_increment),
                    };
                    p_bullet->position = p_player->position;
                    bullet_hitbox = get_component(p_bullet, CHITBOXES_T);
                    bullet_hitbox->atk = p_weapon->base_dmg;
                    bullet_hitbox->src = DMGSRC_PLAYER;

                    bullet_life = get_component(p_bullet, CLIFETIMER_T);
                    bullet_life->poison_value = p_weapon->bullet_lifetime;

                    p_cspr = get_component(p_bullet, CSPRITE_T);
                    p_cspr->current_idx = p_weapon->weapon_idx;
                    p_cspr->rotation = (angle + 2 * angle_increment) * 180 / PI;

                    angle -= p_weapon->spread_range;
                    angle_increment += p_weapon->spread_range;
                }

                p_weapon->cooldown_timer = 1.0f / (p_weapon->fire_rate  * (1 + p_weapon->modifiers[0] * 0.1));
                play_sfx(scene->engine, WEAPON1_FIRE_SFX + p_weapon->weapon_idx, true);
            }

            p_pstate->shoot = 0;
        }
    }
}

void global_external_forces_system(Scene_t* scene)
{
    CTransform_t * p_ctransform;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CTRANSFORM_T], ent_idx, p_ctransform)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
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
            }
            else if (p_ent->position.x > data->game_field_size.x)
            {
                p_ent->position.x -= data->game_field_size.x;
            }
            
            if (p_ent->position.y < 0)
            {
                p_ent->position.y += data->game_field_size.y;
            }
            else if (p_ent->position.y > data->game_field_size.y)
            {
                p_ent->position.y -= data->game_field_size.y;
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
                float dist = Vector2Distance(p_ent->position, target_positions[i]);

                // On hit
                if (dist < p_ent->size + p_other_ent->size)
                {
                    Vector2 attack_dir = Vector2Subtract(target_positions[i], p_ent->position);
                    p_hurtbox->attack_dir = (Vector2){0,0};
                    p_hurtbox->invuln_timer = p_hurtbox->invuln_time;
                    CLifeTimer_t* p_other_life = get_component(p_other_ent, CLIFETIMER_T);
                    if (p_other_life != NULL)
                    {
                        p_other_life->current_life -= p_hitbox->atk;
                        if (p_other_life->current_life <= 0)
                        {
                            p_hurtbox->attack_dir = attack_dir;
                        }
                    }
                    CTransform_t* p_ct = get_component(p_other_ent, CTRANSFORM_T);

                    if (p_ct != NULL)
                    {
                        float kb = 10 * p_hitbox->knockback * p_hurtbox->kb_mult;
                        if (kb < 200) kb = 200;

                        p_ct->velocity = 
                        //Vector2Add(
                            //p_ct->velocity,
                            Vector2Scale(
                                Vector2Normalize(attack_dir),
                                kb
                            //)
                        );
                    }

                    ParticleEmitter_t emitter = {
                        .spr = NULL,
                        .config = get_emitter_conf(&scene->engine->assets, "part_hit"),
                        .position = {
                            .x = p_ent->position.x,
                            .y = p_ent->position.y,
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
        float speed = 100.0f;
        if (p_ct != NULL)
        {
            angle = atan2f(p_ct->velocity.y, p_ct->velocity.x);
            speed = Vector2Length(p_ct->velocity);
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

                float increment = 2 * PI / p_container->num;
                angle += increment / 2;
                for (uint8_t i = 0; i < p_container->num; ++i)
                {
                    Entity_t* p_enemy = create_enemy(&scene->ent_manager, p_ent->size / SIZE_SPLIT_FACTOR);
                    p_enemy->position = p_ent->position;

                    CTransform_t* enemy_ct = get_component(p_enemy, CTRANSFORM_T);
                    enemy_ct->velocity = (Vector2){
                        speed * cosf(angle),
                        speed * sinf(angle),
                    };
                    angle += increment;
                    if (p_spwn != NULL)
                    {

                        CSpawner_t* p_spawner = get_component(p_spwn->spawner, CSPAWNER_T);
                        p_spawner->child_spawn_logic(p_ent, &p_spawner->data, scene);
                        CSpawn_t* new_spwn = add_component(p_enemy, CSPAWNED_T);
                        new_spwn->spawner = p_spwn->spawner;
                    }
                }
            }
            break;
            case CONTAINER_GOAL:
            {
                LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);
                data->game_state = GAME_ENDED;
            }
            break;
            default:
            break;
        }
    }
}

void player_dir_reset_system(Scene_t* scene)
{
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        p_pstate->player_dir.x = 0;
        p_pstate->player_dir.y = 0;
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

void update_player_emitter_system(Scene_t* scene)
{
    CPlayerState_t* p_pstate;
    unsigned long ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_ent =  get_entity(&scene->ent_manager, ent_idx);
        CEmitter_t* p_emitter = get_component(p_ent, CEMITTER_T);
        Vector2 new_pos = Vector2Add(p_ent->position, p_emitter->offset);

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
        update_emitter_handle_position(&scene->part_sys, p_emitter->handle, new_pos);
        update_emitter_handle_rotation(&scene->part_sys, p_emitter->handle, atan2f(p_pstate->aim_dir.y, p_pstate->aim_dir.x) *180/PI - 180);
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
