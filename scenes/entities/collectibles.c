#include "EC.h"
#include "ent_impl.h"

#include "raymath.h"

Entity_t* create_collectible(EntityManager_t* ent_manager, float size, int32_t value)
{
    Entity_t* p_ent = add_entity(ent_manager, COLLECT_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->size = size;
    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    float angle = GetRandomValue(0, 360) * 1.0f * PI /180;
    p_ct->velocity = Vector2Scale((Vector2){cosf(angle), sinf(angle)}, 300);
    p_ct->shape_factor = 9;
    p_ct->active = true;
    p_ct->edge_b = EDGE_BOUNCE;

    CLifeTimer_t* p_life = add_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 1;
    p_life->max_life = 1;
    p_life->poison_value = 6.0f; 

    CAttract_t* p_attract = add_component(p_ent, CATTRACTOR_T);
    p_attract->attract_idx = 1;

    CMoney_t* p_money = add_component(p_ent, CMONEY_T);
    p_money->value = value;
    return p_ent;
}
