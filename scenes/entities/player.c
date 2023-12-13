#include "ent_impl.h"
Entity_t* create_player(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, PLAYER_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->position = (Vector2){128,128};
    p_ent->size = 8;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->active = true;
    p_ct->wraparound = true;
    p_ct->shape_factor = 1.0f;

    add_component(p_ent, CPLAYERSTATE_T);

    return p_ent;
}
