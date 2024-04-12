#include "ent_impl.h"
#include "../assets_tag.h"
#include <math.h>
static Sprite_t* enemies_sprite_map[5] = {0};
static Sprite_t* boss_sprite_map[3] = {0};

#define MAX_MOMENTUM 3500
Entity_t* create_enemy(EntityManager_t* ent_manager, float size, int32_t value)
{
    if (size == 0) return NULL;

    Entity_t* p_ent = add_entity(ent_manager, ENEMY_ENT_TAG);
    if (p_ent == NULL) return NULL;

    p_ent->size = size;

    CTransform_t* p_ct = add_component(p_ent, CTRANSFORM_T);
    p_ct->active = true;
    p_ct->edge_b = EDGE_BOUNCE;
    p_ct->velocity_cap = MAX_MOMENTUM / size;

    CHitBoxes_t* p_hitbox = add_component(p_ent, CHITBOXES_T);
    p_hitbox->size = size;
    p_hitbox->atk = size / 3.0f;
    p_hitbox->knockback = size;
    p_hitbox->dmg_type = DMG_MELEE;
    p_hitbox->src = DMGSRC_ENEMY;
    p_hitbox->hit_sound = WEAPON1_HIT_SFX;

    CHurtbox_t* p_hurtbox = add_component(p_ent, CHURTBOX_T);
    p_hurtbox->size = size *1.05f;
    p_hurtbox->src = DMGSRC_ENEMY;
    p_hurtbox->kb_mult = 0.5f;
    p_hurtbox->invuln_time = 0.0f;

    CLifeTimer_t* p_life = add_component(p_ent, CLIFETIMER_T);
    p_life->max_life = size / 3.5f;
    p_life->current_life = p_life->max_life;

    CContainer_t* p_container = add_component(p_ent, CCONTAINER_T);
    p_container->item = CONTAINER_ENEMY;
    p_container->num = GetRandomValue(0, 99) < 75 ? 2 : 3; // 25% to spawn more

    if (value > 0)
    {
        CWallet_t* p_wallet = add_component(p_ent, CWALLET_T);
        p_wallet->value = value;
    }

    CSprite_t* p_cspr = add_component(p_ent, CSPRITE_T);
    p_cspr->sprites = enemies_sprite_map;
    p_cspr->current_idx = 0;
    p_cspr->rotation_speed = -30 + 60 * (float)rand() / (float)RAND_MAX; 
    p_cspr->colour = WHITE;

    return p_ent;
}

static unsigned int boss_sprite_logic(Entity_t* ent)
{
    CLifeTimer_t* p_life = get_component(ent, CLIFETIMER_T);
    float life_frac = (float)p_life->current_life/ (float)p_life->max_life;

    if (life_frac > 0.66f) return 0;

    if (life_frac > 0.33f) return 1;

    return 2;
}

Entity_t* create_final_enemy(EntityManager_t* ent_manager, float size, Vector2 pos)
{
    Entity_t* p_finalenemy = create_enemy(ent_manager, size, 0);
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
    p_life->current_life = 400;
    p_life->max_life = 400;
    CHurtbox_t* p_hurtbox = get_component(p_finalenemy, CHURTBOX_T);
    p_hurtbox->kb_mult = 3.0f;
    CAttract_t* p_attract = add_component(p_finalenemy, CATTRACTOR_T);
    p_attract->attract_idx = 1;
    p_attract->attract_factor = -0.7;
    p_attract->range_factor = 5.0;

    CSprite_t* p_cspr = get_component(p_finalenemy, CSPRITE_T);
    p_cspr->sprites = boss_sprite_map;
    p_cspr->transition_func = boss_sprite_logic;
    p_cspr->colour = WHITE;

    return p_finalenemy;
}

Entity_t* create_start_target(EntityManager_t* ent_manager, float size, Vector2 pos)
{
    Entity_t* start_target = create_enemy(ent_manager, size, 0);
    if (start_target == NULL) return NULL;

    start_target->position = pos;

    CContainer_t* p_container = get_component(start_target, CCONTAINER_T);
    p_container->item = CONTAINER_START;
    p_container->num = 1;

    remove_component(start_target, CHITBOXES_T);
    remove_component(start_target, CTRANSFORM_T);

    CLifeTimer_t* p_life = get_component(start_target, CLIFETIMER_T);
    p_life->current_life = 1;
    p_life->max_life = 1;

    CSprite_t* p_cspr = get_component(start_target, CSPRITE_T);
    p_cspr->current_idx = 4;
    p_cspr->colour = WHITE;

    return start_target;
}

bool init_enemies_creation(Assets_t* assets)
{
    Sprite_t* spr = get_sprite(assets, "enm_normal");
    enemies_sprite_map[0] = spr;
    spr = get_sprite(assets, "enm_attract");
    enemies_sprite_map[1] = spr;
    spr = get_sprite(assets, "enm_maw");
    enemies_sprite_map[2] = spr;
    spr = get_sprite(assets, "enm_ai");
    enemies_sprite_map[3] = spr;
    spr = get_sprite(assets, "strt_tgt");
    enemies_sprite_map[4] = spr;

    spr = get_sprite(assets, "finale1");
    boss_sprite_map[0] = spr;

    spr = get_sprite(assets, "finale2");
    boss_sprite_map[1] = spr;

    spr = get_sprite(assets, "finale3");
    boss_sprite_map[2] = spr;
    return true;
}
