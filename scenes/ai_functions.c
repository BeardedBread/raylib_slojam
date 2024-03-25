#include "ai_functions.h"
#include "scenes.h"
#include "raymath.h"
#include "assets_tag.h"
void homing_target_func(Entity_t* self, void* scene)
{
    LevelScene_t* level_scene = (LevelScene_t*)scene; 
    CAIFunction_t* c_ai = get_component(self, CAIFUNC_T);

    CTransform_t* p_ct = get_component(self, CTRANSFORM_T);
    if (p_ct == NULL)
    {
        remove_component(self, CAIFUNC_T);
        return;
    }

    //Entity_t* target = get_entity(&level_scene->scene.ent_manager, c_ai->target_idx);
    //if (target == NULL) return;

    float vel = Vector2Length(p_ct->velocity);
    if (vel > p_ct->velocity_cap)
    {
        p_ct->velocity = Vector2Scale(Vector2Normalize(p_ct->velocity), p_ct->velocity_cap);
    }

    Vector2 self_pos = self->position;
    Vector2 self_vel = p_ct->velocity;

    Entity_t* p_target = get_entity(&level_scene->scene.ent_manager, c_ai->target_idx);
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

    float eta =
        -v_c / c_ai->accl
        + sqrtf(
            v_c * v_c / (c_ai->accl * c_ai->accl)
            + 2 * Vector2Length(to_target) / c_ai->accl
        )
    ;

    Vector2 predict_pos = Vector2Add(target_pos, Vector2Scale(target_vel, eta));

    Vector2 to_predict = Vector2Subtract(predict_pos, self_pos);
    p_ct->accel = Vector2Scale(
        Vector2Normalize(to_predict),
        c_ai->accl
    );

    CSprite_t* p_cspr = get_component(self, CSPRITE_T);
    if (p_cspr != NULL)
    {
        float angle = atan2f(to_target.y, to_target.x);
        p_cspr->rotation = angle * 180 / PI;
    }
}

static const uint8_t scale_factors[4][5] = {
    {2,1,1,1,3},
    {1,1,4,1,4},
    {1,2,3,1,6},
    {3,1,1,1,4},
};

static inline unsigned long find_closest_bullet(Scene_t* scene, Vector2 pos)
{
    Entity_t* p_bullet;
    float shortest_dist = 200.0f;
    unsigned long target_idx = MAX_ENTITIES;
    sc_map_foreach_value(&scene->ent_manager.entities_map[BULLET_ENT_TAG], p_bullet)
    {
        if (!p_bullet->m_alive) continue;

        CHitBoxes_t* p_hitbox = get_component(p_bullet, CHITBOXES_T);
        if (p_hitbox->src == DMGSRC_ENEMY) continue;

        CTransform_t* p_bullet_ct = get_component(p_bullet, CTRANSFORM_T);
        Vector2 pred_pos = Vector2Add(
            p_bullet->position,
            Vector2Scale(p_bullet_ct->velocity, scene->delta_time)
        );
        float curr_dist = Vector2Distance(pred_pos, pos);
        if (curr_dist < shortest_dist)
        {
            shortest_dist = curr_dist;
            target_idx = p_bullet->m_id;
        }
    }
    return target_idx;
}

void test_ai_func(Entity_t* self, void* data)
{
    LevelScene_t* level_scene = (LevelScene_t*)data; 
    CAIFunction_t* c_ai = get_component(self, CAIFUNC_T);
    CTransform_t* p_ct = get_component(self, CTRANSFORM_T);
    CLifeTimer_t* p_life = get_component(self, CLIFETIMER_T);

    Entity_t* target = get_entity(&level_scene->scene.ent_manager, c_ai->target_idx);

    if (target == NULL) return;

    if (
        Vector2DistanceSqr(self->position, target->position) < 150 * 150
        && p_life->current_life < 20
    )
    {
        homing_target_func(self, data);
        return;
    }


    CPlayerState_t* p_pstate = get_component(target, CPLAYERSTATE_T);
    if (p_pstate == NULL) return;

    Vector2 target_dir = Vector2Normalize(Vector2Subtract(self->position, target->position));
    const uint8_t (*factors)[5] = scale_factors;
    CWeapon_t* p_wep = get_component(target, CWEAPON_T);
    if (p_wep != NULL)
    {
        factors += p_wep->weapon_idx;
    }

    // Avoid direct sight
    Vector2 rotated_aim = Vector2Rotate(p_pstate->aim_dir, PI / 2.0f);
    float dot_mag = Vector2DotProduct(target_dir, rotated_aim);
    int8_t side_check = dot_mag > 0 ? 1:-1;
    if (side_check ^ c_ai->last_side)
    {
        c_ai->counter = 0;
    }
    else
    {
        c_ai->counter += level_scene->scene.delta_time;
    }
    if (c_ai->counter > 0.1f)
    {
        c_ai->side = side_check;
    }
    c_ai->last_side = side_check;

    float mag = Vector2DotProduct(target_dir, p_pstate->aim_dir);
    mag = mag > 0 ? mag : 0.0f;
    mag *= mag * 400.0f * (*factors)[0];
    p_ct->accel = Vector2Scale(Vector2Rotate(p_pstate->aim_dir, (PI / 2.0f) * c_ai->side), mag);

    // Try to be behind player
    Vector2 target_back = Vector2Add(target->position, Vector2Scale(p_pstate->aim_dir, -target->size * (dot_mag + 1.0f)));
    Vector2 err = Vector2Subtract(self->position, target_back);
    Vector2 accel_to_back = Vector2Scale(err, -2.5f * (*factors)[1]);
    p_ct->accel = Vector2Add(accel_to_back, p_ct->accel);

    // Try to get away from player, decaying magnitude
    float dist_to_target = Vector2Distance(self->position, target->position) - 512.0f;
    mag = 1000.0f * (1.0f / (1.0f + expf(dist_to_target)) - 0.5f);
    p_ct->accel = Vector2Add(
        p_ct->accel,
        Vector2Scale(target_dir, mag * (*factors)[2])
    );

    // Try to stay away from walls
    Vector2 wall_dist = {
        .x = fmin(self->position.x, level_scene->data.game_field_size.x - self->position.x),
        .y = fmin(self->position.y, level_scene->data.game_field_size.y - self->position.y)
    };

    Vector2 wall_accel = {0};
    wall_accel.x = expf(-wall_dist.x/128.0f);
    if (self->position.x > level_scene->data.game_field_size.x /2) wall_accel.x *= -1;

    wall_accel.y = expf(-wall_dist.y/128.0f);
    if (self->position.y > level_scene->data.game_field_size.y /2) wall_accel.y *= -1;

    p_ct->accel = Vector2Add(
        p_ct->accel,
        Vector2Scale(wall_accel, 1200.0f)
    );

    unsigned long bullet_idx = find_closest_bullet(&level_scene->scene, self->position);

    if (bullet_idx != MAX_ENTITIES)
    {
        Entity_t* p_bullet = get_entity(&level_scene->scene.ent_manager, bullet_idx);
        Vector2 bullet_dir = Vector2Normalize(Vector2Subtract(self->position, p_bullet->position));
        CTransform_t* p_bullet_ct = get_component(p_bullet, CTRANSFORM_T);

        Vector2 rotated_aim = Vector2Normalize(Vector2Rotate(p_bullet_ct->velocity, PI / 2.0f));
        float dot_mag = Vector2DotProduct(bullet_dir, rotated_aim);
        int8_t side_check = dot_mag > 0 ? 1:-1;

        p_ct->accel = Vector2Add(p_ct->accel,
            Vector2Scale(Vector2Rotate(Vector2Normalize(p_bullet_ct->velocity), (PI / 2.0f) * side_check), 1500.0f)
            );
    }

    mag = Vector2Length(p_ct->accel);
    float true_mag = (mag > c_ai->accl) ? c_ai->accl : mag;

    p_ct->accel = Vector2Scale(p_ct->accel, true_mag / mag);

    // Shooting logic
    //
    float facing_angle = atan2f(-target_dir.y, -target_dir.x) * 180 / PI;
    CWeapon_t* p_weapon = get_component(self, CWEAPON_T);
    if (p_weapon->cooldown_timer <= 0)
    {
        Entity_t* p_bullet = create_bullet(&level_scene->scene.ent_manager);
        p_bullet->position = self->position;
        CTransform_t* bullet_ct = get_component(p_bullet, CTRANSFORM_T);
        bullet_ct->velocity = Vector2Scale(target_dir, -p_weapon->proj_speed);
        CHitBoxes_t* bullet_hitbox = get_component(p_bullet, CHITBOXES_T);
        bullet_hitbox->atk = p_weapon->base_dmg;
        bullet_hitbox->src = DMGSRC_ENEMY;
        bullet_hitbox->hit_sound = WEAPON1_HIT_SFX;
        bullet_hitbox->knockback = p_weapon->bullet_kb;
        bullet_hitbox->dir = target_dir;
        p_weapon->cooldown_timer = 1.0f / (p_weapon->fire_rate);

        CHurtbox_t* p_hurtbox = add_component(p_bullet, CHURTBOX_T);
        p_hurtbox->size = p_bullet->size *1.05f;
        p_hurtbox->src = DMGSRC_ENEMY;

        CSprite_t* p_cspr = get_component(p_bullet, CSPRITE_T);
        p_cspr->colour = RED;
        p_cspr->current_idx = p_weapon->weapon_idx;
        p_cspr->rotation = facing_angle;
        play_sfx(level_scene->scene.engine, WEAPON1_FIRE_SFX, true);
    }

    CSprite_t* p_cspr = get_component(self, CSPRITE_T);
    p_cspr->rotation = facing_angle;
}
