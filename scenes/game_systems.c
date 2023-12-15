#include "game_systems.h"
#include "EC.h"
#include "constants.h"

#include "ent_impl.h"
#include "raymath.h"

void ai_update_system(Scene_t* scene)
{
    CAIFunction_t* p_ai;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CAIFUNC_T], ent_idx, p_ai)
    {
        Entity_t* p_ent = get_entity(&scene->ent_manager, ent_idx);
        p_ai->func[0](p_ent, p_ai->data, scene);
    }
}

void spawned_update_system(Scene_t* scene)
{
    CSpawn_t* p_spwn;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CSPAWNED_T], ent_idx, p_spwn)
    {
        Entity_t* p_ent = get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive)
        {
            CAIFunction_t* p_ai = get_component(p_spwn->spawner, CAIFUNC_T);
            p_ai->func[1](p_ent, p_ai->data, scene);
        }
    }
}

void life_update_system(Scene_t* scene)
{
    CLifeTimer_t* p_life;
    unsigned int ent_idx;
    sc_map_foreach(&scene->ent_manager.component_map[CLIFETIMER_T], ent_idx, p_life)
    {
        if (p_life->current_life <= 0)
        {
            remove_entity(&scene->ent_manager, ent_idx);
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

    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_player = get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_T);
        
        p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL);

        // Mouse aim direction
        if (p_player->m_tag == PLAYER_ENT_TAG)
        {
            Vector2 raw_mouse_pos = GetMousePosition();
            raw_mouse_pos = Vector2Subtract(raw_mouse_pos, (Vector2){data->game_rec.x, data->game_rec.y});
            p_pstate->aim_dir = Vector2Normalize(Vector2Subtract(raw_mouse_pos, p_player->position));
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
                    boost_speed > 400 ? boost_speed : 400
                );
                p_pstate->boost_cooldown = 3.0f;
            }
            else
            {
                p_pstate->boosting &= ~(0b01);
            }
        }
        p_pstate->boosting <<= 1;
        p_pstate->boosting &= 0b11;

        // Shoot
        if (p_pstate->shoot > 0)
        {
            CWeapon_t* p_weapon = get_component(p_player, CWEAPON_T);
            if (p_weapon != NULL && p_weapon->cooldown_timer <= 0)
            {
                Entity_t* p_bullet = create_bullet(&scene->ent_manager);
                CTransform_t* bullet_ct = get_component(p_bullet, CTRANSFORM_T);
                bullet_ct->velocity = Vector2Scale(p_pstate->aim_dir, p_weapon->proj_speed);
                p_bullet->position = p_player->position;

                CHitBoxes_t* bullet_hitbox = get_component(p_bullet, CHITBOXES_T);
                bullet_hitbox->atk = p_weapon->base_dmg;

                p_weapon->cooldown_timer = 1.0f / p_weapon->fire_rate;
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

        //float mag = Vector2Length(p_ctransform->velocity);
        //p_ctransform->velocity = Vector2Scale(
        //    Vector2Normalize(p_ctransform->velocity),
        //    (mag > PLAYER_MAX_SPEED)? PLAYER_MAX_SPEED:mag
        //);

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

void hitbox_update_system(Scene_t* scene)
{
    static bool checked_entities[MAX_COMP_POOL_SIZE] = {0};
    LevelSceneData_t* data = &(((LevelScene_t*)scene)->data);

    unsigned int ent_idx;
    CHitBoxes_t* p_hitbox;
    sc_map_foreach(&scene->ent_manager.component_map[CHITBOXES_T], ent_idx, p_hitbox)
    {
        Entity_t *p_ent =  get_entity(&scene->ent_manager, ent_idx);
        if (!p_ent->m_alive) continue;

        memset(checked_entities, 0, sizeof(checked_entities));

        unsigned int other_ent_idx;
        CHurtbox_t* p_hurtbox;
        CLifeTimer_t* p_life = get_component(p_ent, CLIFETIMER_T);
        sc_map_foreach(&scene->ent_manager.component_map[CHURTBOX_T], other_ent_idx, p_hurtbox)
        {
            if (other_ent_idx == ent_idx) continue;
            if (checked_entities[other_ent_idx]) continue;

            Entity_t* p_other_ent = get_entity(&scene->ent_manager, other_ent_idx);
            if (!p_other_ent->m_alive) continue;

            float dist = Vector2Distance(p_ent->position, p_other_ent->position);

            if (dist < p_ent->size + p_other_ent->size)
            {
                CLifeTimer_t* p_other_life = get_component(p_other_ent, CLIFETIMER_T);
                if (p_other_life != NULL)
                {
                    p_other_life->current_life -= p_hitbox->atk;
                }
                if (p_life != NULL)
                {
                    p_life->current_life--;
                    if (p_life->current_life <= 0)
                    {
                        break;
                    }
                }
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
        CSpawn_t* p_spwn = get_component(p_ent, CSPAWNED_T);
        float angle = 0.0f;
        float speed = 100.0f;
        if (p_ct != NULL)
        {
            angle = atan2f(p_ct->velocity.y, p_ct->velocity.x);
            speed = Vector2Length(p_ct->velocity);
        }

        switch (p_container->item)
        {
            case CONTAINER_ENEMY:
            {
                if (p_ent->size <= 8 || p_container->num == 0) break;

                float increment = 2 * PI / p_container->num;
                angle += increment / 2;
                for (uint8_t i = 0; i < p_container->num; ++i)
                {
                    Entity_t* p_enemy = create_enemy(&scene->ent_manager, p_ent->size / 2);
                    p_enemy->position = p_ent->position;

                    CTransform_t* enemy_ct = get_component(p_enemy, CTRANSFORM_T);
                    enemy_ct->velocity = (Vector2){
                        speed * cosf(angle),
                        speed * sinf(angle),
                    };
                    angle += increment;
                    if (p_spwn != NULL)
                    {

                        CAIFunction_t* p_ai = get_component(p_spwn->spawner, CAIFUNC_T);
                        p_ai->func[2](p_ent, p_ai->data, scene);
                        CSpawn_t* new_spwn = add_component(p_enemy, CSPAWNED_T);
                        new_spwn->spawner = p_spwn->spawner;
                    }
                }
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

