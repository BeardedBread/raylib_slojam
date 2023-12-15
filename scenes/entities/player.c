#include "EC.h"
#include "ent_impl.h"
Entity_t* create_player(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, PLAYER_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->position = (Vector2){128,128};
    p_ent->size = 8;

    CHurtbox_t* p_hurtbox = add_component(p_ent, CHURTBOX_T);
    p_hurtbox->size = 8;
    p_hurtbox->src = DMGSRC_PLAYER;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->active = true;
    p_ct->edge_b = EDGE_WRAPAROUND;
    p_ct->shape_factor = 1.0f;

    add_component(p_ent, CPLAYERSTATE_T);

    CWeapon_t* p_weapon = add_component(p_ent, CWEAPON_T);
    p_weapon->base_dmg = 5;
    p_weapon->fire_rate = 5.5f;
    p_weapon->proj_speed = 800;

    CLifeTimer_t* p_life = add_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 30;
    p_life->max_life = 30;

    add_component(p_ent, CPLAYERSTATE_T);
    return p_ent;
}
