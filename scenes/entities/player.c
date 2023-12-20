#include "EC.h"
#include "ent_impl.h"

CWeapon_t all_weapons[3] = {
    // Pistol
    {
        .base_dmg = 10, .proj_speed = 800, .fire_rate = 4.5f,
        .cooldown_timer = 0, .spread_range = 0, .n_bullets = 1,
        .bullet_lifetime = 0, .weapon_idx = 0, .homing = false,
    },
    // Shotgun
    {
        .base_dmg = 5, .proj_speed = 1100, .fire_rate = 1.3f,
        .cooldown_timer = 0, .spread_range = 5*PI/180, .n_bullets = 7,
        .bullet_lifetime = 4.6f * 60.0f, .weapon_idx = 1, .homing = false,
    },
    // Homing Rockets
    {
        .base_dmg = 7, .proj_speed = 500, .fire_rate = 2.6f,
        .cooldown_timer = 0, .spread_range = 0, .n_bullets = 1,
        .bullet_lifetime = 3.0f, .weapon_idx = 2, .homing = true,
    },
};

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

    CWeaponStore_t* p_weaponstore = add_component(p_ent, CWEAPONSTORE_T);
    memcpy(p_weaponstore->weapons, all_weapons, sizeof(all_weapons));
    p_weaponstore->n_weapons = 3;
    p_weaponstore->unlocked[0] = true;
    p_weaponstore->unlocked[1] = true;
    p_weaponstore->unlocked[2] = true;

    CWeapon_t* p_weapon = add_component(p_ent, CWEAPON_T);
    *p_weapon = p_weaponstore->weapons[0];

    CLifeTimer_t* p_life = add_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 30;
    p_life->max_life = 30;
    
    add_component(p_ent, CPLAYERSTATE_T);
    return p_ent;
}
