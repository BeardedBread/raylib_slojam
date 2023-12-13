#include "game_systems.h"
#include "EC.h"
#include "constants.h"

#include "ent_impl.h"
#include "raymath.h"

void player_movement_input_system(Scene_t* scene)
{
    CPlayerState_t* p_pstate;
    unsigned int ent_idx;

    sc_map_foreach(&scene->ent_manager.component_map[CPLAYERSTATE_T], ent_idx, p_pstate)
    {
        Entity_t* p_player = get_entity(&scene->ent_manager, ent_idx);
        CTransform_t* p_ctransform = get_component(p_player, CTRANSFORM_T);
        
        p_ctransform->accel = Vector2Scale(Vector2Normalize(p_pstate->player_dir), MOVE_ACCEL);
        p_ctransform->accel = Vector2Scale(p_ctransform->accel, p_pstate->boosting? 2:1);
        if (p_pstate->boosting == 0b01)
        {
            Vector2 mouse_pos = GetMousePosition();
            Vector2 point_dir = Vector2Subtract(mouse_pos, p_player->position);
            p_ctransform->velocity = Vector2Scale(
                Vector2Normalize(point_dir),
                Vector2Length(p_ctransform->velocity)
            );
        }
        p_pstate->boosting <<= 1;
        p_pstate->boosting &= 0b11;

        // Mouse aim direction
        p_pstate->aim_dir = Vector2Normalize(Vector2Subtract(GetMousePosition(), p_player->position));

        // Shoot
        if (p_pstate->shoot > 0)
        {
            Entity_t* p_bullet = create_bullet(&scene->ent_manager);
            CTransform_t* bullet_ct = get_component(p_bullet, CTRANSFORM_T);
            bullet_ct->velocity = Vector2Scale(p_pstate->aim_dir, 300);
            p_bullet->position = p_player->position;
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
            Vector2Scale(p_ctransform->velocity, -0.5)
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
        p_ctransform->prev_velocity = p_ctransform->velocity;

        p_ctransform->velocity = Vector2Add(
            p_ctransform->velocity,
            Vector2Scale(p_ctransform->accel, delta_time)
        );

        float mag = Vector2Length(p_ctransform->velocity);
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

        // Level boundary collision
        //if (p_ent->position.x - p_ent->size < 0)
        //{
        //    p_ent->position.x =  p_ent->size;
        //    p_ctransform->velocity.x = 0;
        //}
        //else if (p_ent->position.x + p_ent->size > data->game_field_size.x)
        //{
        //    p_ent->position.x =  data->game_field_size.x - p_ent->size;
        //    p_ctransform->velocity.x = 0;
        //}
        //
        //if (p_ent->position.y - p_ent->size < 0)
        //{
        //    p_ent->position.y =  p_ent->size;
        //    p_ctransform->velocity.y = 0;
        //}
        //else if (p_ent->position.y + p_ent->size > data->game_field_size.y)
        //{
        //    p_ent->position.y =  data->game_field_size.y - p_ent->size;
        //    p_ctransform->velocity.y = 0;
        //}
        if (p_ctransform->wraparound)
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
        else
        {
            if (p_ent->position.x < 0
             || (p_ent->position.x > data->game_field_size.x)
            
             || (p_ent->position.y < 0)
             || (p_ent->position.y > data->game_field_size.y)
             )
            {
                remove_entity(&scene->ent_manager, p_ent->m_id);
            }
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

