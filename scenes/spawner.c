#include "engine.h"
#include "level_ent.h"
#include "assets_tag.h"
#include "constants.h"
#include "scenes.h"
#include "ai_functions.h"
#include "raymath.h"

struct SpawnVariantData {
    uint16_t max_spawn;
    uint8_t prob;
};

struct RankSpawnData {
    uint32_t rank_up_value;
    float spawn_time_range[2];
    uint8_t size_chances[4];
    uint8_t speed_chances[3];
    uint8_t max_spawns;
    float drop_modifier;
    struct SpawnVariantData variant_data[3];
};

static const uint32_t SIZE_RANGES[5] = {12,18,32,56,72};
static const uint32_t SPEED_RANGES[4] = {100,200,250,320};
static const struct RankSpawnData RANK_DATA[MAX_RANK] = {
    {50  , {5,1}, {10,100,100,100}, {100,100,100}, 7 , 1.0f, {{0 ,0 }, {0,0 },{0,0}}},
    {150 , {4,1}, {0 ,100,100,100}, {70 ,100,100}, 12, 1.1f, {{0 ,0 }, {0,0 },{0,0}}},
    {250 , {3,2}, {0 ,85 ,100,100}, {15 ,100,100}, 13, 1.2f, {{3 ,10}, {0,0 },{0,0}}},
    {350 , {3,2}, {0 ,25 ,90,100}, {0  ,85 ,100}, 15, 1.3f, {{5 ,10}, {3,5 },{0,0}}},
    {500 , {3,2}, {0 ,15 ,95 ,100}, {10 ,65 ,100}, 17, 1.5f, {{7 ,15}, {5,8 },{0,0}}},
    {650 , {3,1}, {0 ,10  ,90 ,100}, {0  ,85 ,100}, 20, 1.7f, {{9,18}, {6,8 },{1,10}}},
    {850, {2,2}, {0 ,10 ,85 ,100}, {10 ,70 ,100}, 24, 2.0f, {{10,20}, {7,10},{1,10}}},
    {1400, {2,1}, {0 ,10  ,70 ,100}, {0  ,60 ,100}, 27, 2.2f, {{11,25}, {8,10},{1,15}}},
    {2600, {2,1}, {0 ,0  ,60 ,100}, {0  ,45 ,100}, 31, 2.5f, {{12,25}, {10,12},{1,15}}},
    {9999, {1,1}, {0 ,0  ,50 ,100}, {0  ,30 ,100}, 44, 3.0f, {{20,25}, {20,15},{2,20}}},
};

#define ATTRACT_RANK 6

static inline void make_enemy_ai(Entity_t* ai_enemy)
{
    ai_enemy->size = 16;
    CAIFunction_t* c_ai = add_component(ai_enemy, CAIFUNC_T);
    c_ai->target_tag = PLAYER_ENT_TAG;
    c_ai->target_idx = MAX_ENTITIES;
    c_ai->accl = 2000.0;
    c_ai->func = &test_ai_func;
    CLifeTimer_t* p_life = get_component(ai_enemy, CLIFETIMER_T);
    p_life->current_life = 50;
    p_life->max_life = 50;

    CTransform_t* p_ct = get_component(ai_enemy, CTRANSFORM_T);
    p_ct->shape_factor = 5.0f;
    p_ct->velocity_cap = 600;

    CSprite_t* p_cspr = get_component(ai_enemy, CSPRITE_T);
    p_cspr->current_idx = 3;

    CWeapon_t* p_weapon = add_component(ai_enemy, CWEAPON_T);
    *p_weapon = (CWeapon_t){
    .base_dmg = 5, .proj_speed = 500, .fire_rate = 1.0f, .bullet_kb = 3.3f,
    .cooldown_timer = 3.0f, .spread_range = 0, .n_bullets = 1,
    .bullet_lifetime = 0, .weapon_idx = 0, .special_prop = 0x0,
    };

    CWallet_t* p_wallet = get_component(ai_enemy, CWALLET_T);
    p_wallet->value *= 1.5f;
}

static inline void make_enemy_maws(Entity_t* p_ent)
{
    CAIFunction_t* p_ai = add_component(p_ent, CAIFUNC_T);
    p_ai->target_idx = MAX_ENTITIES;
    p_ai->target_tag = PLAYER_ENT_TAG;
    p_ai->func = &homing_target_func;
    p_ai->accl = 1000;
    p_ai->lag_time = 0.1f;

    CLifeTimer_t* p_life = get_component(p_ent, CLIFETIMER_T);
    p_life->current_life = 1;
    p_life->max_life = 1;

    CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
    p_ct->shape_factor = 2;
    p_ct->velocity_cap = 800;
    //p_ct->velocity = Vector2Scale(p_ct->velocity, 2);

    CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
    p_cspr->current_idx = 2;
    remove_component(p_ent, CCONTAINER_T);

    p_ent->size = p_ent->size < 16 ? 16 : p_ent->size;
}

static inline void make_enemy_attract(Entity_t* p_ent)
{
    CAttract_t* p_attract = add_component(p_ent, CATTRACTOR_T);
    p_attract->attract_idx = 1;
    p_attract->attract_factor = 0.05f;
    p_attract->range_factor = 7.0f;

    CSprite_t* p_cspr = get_component(p_ent, CSPRITE_T);
    p_cspr->current_idx = 1;
}

void split_spawn_logic_func(Entity_t* new_ent, SpawnerData* data, void* scene)
{
    data->spawned++;
    if (GetRandomValue(0, 100) < RANK_DATA[data->rank].variant_data[1].prob) // Flat percent
    {
        make_enemy_maws(new_ent);
    }
    else if (data->rank >= ATTRACT_RANK)
    {
        make_enemy_attract(new_ent);
    }
}

void despawn_logic_func(Entity_t* self, SpawnerData* data, void* scene)
{
    CTransform_t* p_ct = get_component(self, CTRANSFORM_T);
    data->rank_counter += 1 + 50 / self->size + Vector2Length(p_ct->velocity) / 1000;
    if (data->spawned > 0)
    {
        data->spawned--;
    }

    if (data->spawned == 0)
    {
        data->spawn_timer = 0;
    }

    if (get_component(self, CATTRACTOR_T) != NULL)
    {
        data->custom_counter[0]--;
    }
    if (get_component(self, CWEAPON_T) != NULL)
    {
        data->custom_counter[1]--;
        data->spawned--;
    }
}

void set_spawn_level(SpawnerData* spwn_data, uint8_t lvl)
{
    spwn_data->rank = lvl;
    spwn_data->rank_counter = 0;
    spwn_data->rank_timer = 0;
    spwn_data->next_rank_count = RANK_DATA[lvl].rank_up_value;
}

void spawn_logic_func(Entity_t* self, SpawnerData* spwn_data, void* scene)
{
    LevelScene_t* lvl_scene = (LevelScene_t*)scene;
    if (spwn_data->rank < MAX_RANK - 1)
    {
        spwn_data->rank_timer += lvl_scene->scene.delta_time;
        if (spwn_data->rank_timer > 1.0f)
        {
            spwn_data->rank_counter += 1 + spwn_data->rank / 2 + spwn_data->rank / 4; // +1 every sec
            spwn_data->rank_timer -= 1.0f;
        }
        if (spwn_data->rank_counter >= RANK_DATA[spwn_data->rank].rank_up_value)
        {
            spwn_data->rank_counter -= RANK_DATA[spwn_data->rank].rank_up_value;
            spwn_data->rank++;
            spwn_data->next_rank_count = RANK_DATA[spwn_data->rank].rank_up_value;
            play_sfx(lvl_scene->scene.engine, RANKUP_SFX, false);
        }
    }

    if (spwn_data->spawned >= RANK_DATA[spwn_data->rank].max_spawns) return;

    spwn_data->spawn_timer -= lvl_scene->scene.delta_time;
    if (spwn_data->spawn_timer <= 0.0f)
    {
        int chance = GetRandomValue(0, 100);
        uint8_t idx = 0;
        for (idx = 0; idx < 3; idx++)
        {
            if (chance <= RANK_DATA[spwn_data->rank].size_chances[idx]) break;
        }
        float enemy_sz = GetRandomValue(SIZE_RANGES[idx], SIZE_RANGES[idx+1]);

        chance = GetRandomValue(0, 100);
        for (idx = 0; idx < 2; idx++)
        {
            if (chance <= RANK_DATA[spwn_data->rank].speed_chances[idx]) break;
        }
        float speed = SPEED_RANGES[idx]
            + (SPEED_RANGES[idx+1] - SPEED_RANGES[idx])
            * (float)rand() / (float)RAND_MAX;

        spwn_data->spawned++;
        spwn_data->spawn_timer += RANK_DATA[spwn_data->rank].spawn_time_range[0]
                + RANK_DATA[spwn_data->rank].spawn_time_range[1] * (float)rand() / (float)RAND_MAX;

        float value = 1.0; // always have 1
        value += enemy_sz / 3 + speed / 200.0;
        value *= RANK_DATA[spwn_data->rank].drop_modifier;
        Entity_t* p_ent = create_enemy(&lvl_scene->scene.ent_manager, enemy_sz, (int32_t)value);

        CSpawn_t* p_spwn = add_component(p_ent, CSPAWNED_T);
        p_spwn->spawner = self;

        CTransform_t* p_ct = get_component(p_ent, CTRANSFORM_T);
        // Pick a side
        uint8_t side = GetRandomValue(0, 3);
        float angle = 90.0f * side;
        switch (side)
        {
            case 0:
                p_ent->position = (Vector2){
                    -80,
                    GetRandomValue(enemy_sz, lvl_scene->data.game_field_size.y - enemy_sz)
                };
            break;
            case 1:
                p_ent->position = (Vector2){
                    GetRandomValue(enemy_sz, lvl_scene->data.game_field_size.x - enemy_sz),
                    -80
                };
            break;
            case 2:
                p_ent->position = (Vector2){
                    lvl_scene->data.game_field_size.x + 80,
                    GetRandomValue(enemy_sz, lvl_scene->data.game_field_size.y - enemy_sz)
                };
            break;
            default:
                p_ent->position = (Vector2){
                    GetRandomValue(enemy_sz, lvl_scene->data.game_field_size.x - enemy_sz),
                    lvl_scene->data.game_field_size.y + 80
                };
            break;
        }

        angle += GetRandomValue(-30, 30);
        angle = angle * PI / 180;
        p_ct->velocity = (Vector2){
            speed * cosf(angle),
            speed * sinf(angle),
        };

        if (
            spwn_data->custom_counter[1] < RANK_DATA[spwn_data->rank].variant_data[2].max_spawn
            && spwn_data->spawned > 1
            && GetRandomValue(0, 100) < RANK_DATA[spwn_data->rank].variant_data[2].prob
        )
        {
            make_enemy_ai(p_ent);
            spwn_data->custom_counter[1]++;
            spwn_data->spawned++;
        }
        else if (spwn_data->rank >= ATTRACT_RANK)
        {
            make_enemy_attract(p_ent);
        }

    }

}

Entity_t* create_spawner(EntityManager_t* ent_manager)
{
    Entity_t* p_ent = add_entity(ent_manager, UTIL_ENT_TAG);
    if (p_ent == NULL) return NULL;

    CSpawner_t* p_spawner = add_component(p_ent, CSPAWNER_T);
    p_spawner->spawner_main_logic = &spawn_logic_func;
    p_spawner->spawnee_despawn_logic = &despawn_logic_func;
    p_spawner->child_spawn_logic = &split_spawn_logic_func;
    p_spawner->data.next_rank_count = RANK_DATA[0].rank_up_value;
    return p_ent;
}
