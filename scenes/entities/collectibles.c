#include "EC.h"
#include "ent_impl.h"
#include "../assets_tag.h"

#include "raymath.h"

static Sprite_t* collectible_sprites[1] = {0};

Entity_t* create_collectible(EntityManager_t* ent_manager, float size, int32_t value)
{
    Entity_t* p_ent = add_entity(ent_manager, COLLECT_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->size = size;
    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->velocity_cap = 1000;
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
    p_attract->attract_factor = 1.5f;
    p_attract->range_factor = 1;

    CMoney_t* p_money = add_component(p_ent, CMONEY_T);
    p_money->value = value;
    p_money->collect_sound = COLLECT_SFX;
    
    return p_ent;
}

Entity_t* create_end_item(EntityManager_t* ent_manager, float size)
{
    Entity_t* p_ent = create_collectible(ent_manager, size, 0);
    if (p_ent == NULL) return NULL;
    CContainer_t* p_container = add_component(p_ent, CCONTAINER_T);

    p_container->item = CONTAINER_ENDING;

    CMoney_t* p_money = get_component(p_ent, CMONEY_T);
    p_money->collect_sound = ENDING_SFX;

    CSprite_t* p_cspr = add_component(p_ent, CSPRITE_T);
    p_cspr->sprites = collectible_sprites;
    p_cspr->current_idx = 0;
    p_cspr->colour = WHITE;

    CLifeTimer_t* p_life = get_component(p_ent, CLIFETIMER_T);
    p_life->poison_value = 0.0f; 

    CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
    p_ct->edge_b = EDGE_WRAPAROUND;
    return p_ent;
}

bool init_collectible_creation(Assets_t* assets)
{
    Sprite_t* spr = get_sprite(assets, "upg6_icon");
    collectible_sprites[0] = spr;

    return true;
}
