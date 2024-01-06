#include "EC.h"
#include "ent_impl.h"

static Sprite_t* bullet_sprite_map[3] = {0};

Entity_t* create_bullet(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, NO_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->size = 6;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->active = true;

    CHitBoxes_t* p_hitbox = add_component(p_ent, CHITBOXES_T);
    p_hitbox->size = 6;
    p_hitbox->knockback = 0.0f;
    p_hitbox->dmg_type = DMG_PROJ;
    p_hitbox->hit_sound = N_SFX;

    CLifeTimer_t* p_life = add_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 1;
    p_life->max_life = 1;

    CSprite_t* p_cspr = add_component(p_ent, CSPRITE_T);
    p_cspr->sprites = bullet_sprite_map;
    p_cspr->current_idx = 0;

    return p_ent;
}

bool init_bullet_creation(Assets_t* assets)
{
    Sprite_t* spr = get_sprite(assets, "blt1");
    bullet_sprite_map[0] = spr;

    spr = get_sprite(assets, "blt2");
    bullet_sprite_map[1]= spr;

    spr = get_sprite(assets, "blt3");
    bullet_sprite_map[2]= spr;
    return true;
}
