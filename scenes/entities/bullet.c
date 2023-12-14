#include "EC.h"
#include "ent_impl.h"

Entity_t* create_bullet(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, NO_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->size = 4;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->active = true;

    CHitBoxes_t* p_hitbox = add_component(p_ent, CHITBOXES_T);
    p_hitbox->size = 4;
    p_hitbox->dmg_type = DMG_PROJ;

    return p_ent;
}
