#include "ent_impl.h"

static Sprite_t* enemies_sprite_map[1] = {0};

Entity_t* create_enemy(EntityManager_t* ent_manager, float size)
{
    Entity_t* p_ent = add_entity(ent_manager, ENEMY_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->size = size;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->active = true;
    p_ct->edge_b = EDGE_BOUNCE;

    CHitBoxes_t* p_hitbox = add_component(p_ent, CHITBOXES_T);
    p_hitbox->size = size;
    p_hitbox->atk = size / 4;
    p_hitbox->knockback = size;
    p_hitbox->dmg_type = DMG_MELEE;
    p_hitbox->src = DMGSRC_ENEMY;
    p_hitbox->hit_sound = N_SFX;

    CHurtbox_t* p_hurtbox = add_component(p_ent, CHURTBOX_T);
    p_hurtbox->size = size;
    p_hurtbox->src = DMGSRC_ENEMY;
    p_hurtbox->kb_mult = 0.5f;

    CLifeTimer_t* p_life = add_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 4;
    p_life->max_life = 4;

    CContainer_t* p_container = add_component(p_ent, CCONTAINER_T);
    p_container->item = CONTAINER_ENEMY;
    p_container->num = 2;

    CSprite_t* p_cspr = add_component(p_ent, CSPRITE_T);
    p_cspr->sprites = enemies_sprite_map;
    p_cspr->current_idx = 0;
    p_cspr->rotation_speed = -30 + 60 * (float)rand() / (float)RAND_MAX; 

    return p_ent;
}

bool init_enemies_creation(Assets_t* assets)
{
    Sprite_t* spr = get_sprite(assets, "enm_normal");
    enemies_sprite_map[0] = spr;

    return true;
}
