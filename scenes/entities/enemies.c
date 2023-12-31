#include "ent_impl.h"
#include "../assets_tag.h"
#include <math.h>
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
    p_hitbox->hit_sound = WEAPON1_HIT_SFX;

    CHurtbox_t* p_hurtbox = add_component(p_ent, CHURTBOX_T);
    p_hurtbox->size = size;
    p_hurtbox->src = DMGSRC_ENEMY;
    p_hurtbox->kb_mult = 0.5f;
    p_hurtbox->invuln_time = 0.0f;

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

Entity_t* create_final_enemy(EntityManager_t* ent_manager, float size, Vector2 pos)
{
    Entity_t* p_finalenemy = create_enemy(ent_manager, size);
    if (p_finalenemy == NULL) return NULL;

    CContainer_t* p_container = get_component(p_finalenemy, CCONTAINER_T);
    p_container->item = CONTAINER_GOAL;
    p_container->num = 1;
    remove_component(p_finalenemy, CHITBOXES_T);

    p_finalenemy->position = pos;
    float angle = 2 * PI * (float)rand() / (float)RAND_MAX;
    CTransform_t* p_ct = get_component(p_finalenemy, CTRANSFORM_T);
    p_ct->edge_b = EDGE_WRAPAROUND;
    p_ct->velocity = (Vector2){
        50 * cos(angle), 50 * sin(angle)
    };
    CLifeTimer_t* p_life = get_component(p_finalenemy, CLIFETIMER_T);
    p_life->current_life = 300;
    p_life->max_life = 300;
    CHurtbox_t* p_hurtbox = get_component(p_finalenemy, CHURTBOX_T);
    p_hurtbox->kb_mult = 3.0f;
    CAttract_t* p_attract = add_component(p_finalenemy, CATTRACTOR_T);
    p_attract->attract_idx = 1;
    p_attract->attract_factor = -1;
    p_attract->range_factor = 5.0;

    return p_finalenemy;
}

bool init_enemies_creation(Assets_t* assets)
{
    Sprite_t* spr = get_sprite(assets, "enm_normal");
    enemies_sprite_map[0] = spr;

    return true;
}
