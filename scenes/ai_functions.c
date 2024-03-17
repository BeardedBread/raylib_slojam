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

