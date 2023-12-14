#include "ent_impl.h"

Entity_t* create_enemy(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, ENEMY_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->size = 10;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->active = true;

    CHurtbox_t* p_hurtbox = add_component(p_ent, CHURTBOX_T);
    p_hurtbox->size = 10;

    CLifeTimer_t* p_life = add_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 4;
    p_life->max_life = 4;

    return p_ent;
}

