#include "ai_functions.h"
#include "scenes.h"
#include "raymath.h"
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
void test_ai_func(Entity_t* self, void* data)
{
    LevelScene_t* level_scene = (LevelScene_t*)data; 
    CAIFunction_t* c_ai = get_component(self, CAIFUNC_T);
    CTransform_t* p_ct = get_component(self, CTRANSFORM_T);
    p_ct->accel = (Vector2){0,0};

    Entity_t* target = get_entity(&level_scene->scene.ent_manager, c_ai->target_idx);

    if (target == NULL) return;

    CPlayerState_t* p_pstate = get_component(target, CPLAYERSTATE_T);
    if (p_pstate == NULL) return;

    Vector2 target_dir = Vector2Normalize(Vector2Subtract(self->position, target->position));
    // Avoid direct sight
    Vector2 rotated_aim = Vector2Rotate(p_pstate->aim_dir, PI / 2.0f);
    float side_check = Vector2DotProduct(target_dir, rotated_aim);
    
    float mag = Vector2DotProduct(target_dir, p_pstate->aim_dir);
    mag = mag > 0 ? mag : 0.0f;
    mag *= mag * 1000.0f;
    p_ct->accel = Vector2Scale((side_check > 0) ? rotated_aim : Vector2Rotate(p_pstate->aim_dir, -PI / 2.0f), mag);

    // Try to be behind player
    Vector2 target_back = Vector2Add(target->position, Vector2Scale(p_pstate->aim_dir, -target->size * 10.0f));
    Vector2 err = Vector2Subtract(self->position, target_back);
    Vector2 accel_to_back = Vector2Scale(err, -2.2f);
    p_ct->accel = Vector2Add(accel_to_back, p_ct->accel);

    // Try to get away from player, decaying magnitude
    float dist_to_target = Vector2Distance(self->position, target->position) - 512.0f;
    mag = 1000.0f * (1.0f / (1.0f + expf(dist_to_target)) - 0.5f);
    p_ct->accel = Vector2Add(
        p_ct->accel,
        Vector2Scale(target_dir, mag)
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
}
