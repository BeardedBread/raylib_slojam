#include "ent_impl.h"

Entity_t* create_enemy(EntityManager_t* ent_manager, float size)
{
    Entity_t* p_ent = add_entity(ent_manager, ENEMY_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->size = size;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->active = true;
    p_ct->edge_b = EDGE_BOUNCE;

    CHurtbox_t* p_hurtbox = add_component(p_ent, CHURTBOX_T);
    p_hurtbox->size = size;

    CLifeTimer_t* p_life = add_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 4;
    p_life->max_life = 4;

    CContainer_t* p_container = add_component(p_ent, CCONTAINER_T);
    p_container->item = CONTAINER_ENEMY;
    p_container->num = 2;
    return p_ent;
}

