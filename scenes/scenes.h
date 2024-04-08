#ifndef SCENES_H
#define SCENES_H
#include "engine.h"
#include "ent_impl.h"
#include "constants.h"

typedef struct UIButton {
    Rectangle box;
    bool pressed;
    bool hover;
    bool enabled;
} UIButton;

typedef struct StoreItem
{
    int32_t cost;
    int8_t cap;
    int8_t remaining;
    int32_t cost_increment;
    int32_t cost_cap;
} StoreItem;

typedef struct UpgradeBox {
    Sprite_t* spr;
    UIButton button;
    int dot_spacing;
    bool show_dots;
    StoreItem* item;
} UpgradeBox;

typedef struct ShopUI {
    UpgradeBox upgrades[N_UPGRADES];
    Rectangle desc_box;
    int icon_size;
    int dot_size;
    Vector2 pos;
    const char* desc_text;
} ShopUI;

typedef struct UpgradeStoreInventory
{
    StoreItem firerate_upgrade;
    StoreItem projspeed_upgrade;
    StoreItem damage_upgrade;
    StoreItem health_upgrade;
    StoreItem full_heal;
    StoreItem escape;
    StoreItem thumper;
    StoreItem maws;
    StoreItem flux;
} UpgradeStoreInventory;

typedef struct ShopSceneData {
    RenderTexture2D shop_viewport;
    Rectangle shop_rec;
    ShopUI ui;
    bool refresh_rec;
    UpgradeStoreInventory store;
    uint16_t min_cost;
} ShopSceneData;

typedef struct ShopScene {
    Scene_t scene;
    ShopSceneData data;
} ShopScene_t;

typedef enum GameState {
    GAME_STARTING = 0,
    GAME_PLAYING,
    GAME_ENDED
} GameState;

typedef struct LevelCamera {
    Camera2D cam;
    Vector2 target_pos;
    float base_y;
    Vector2 current_vel;
    float mass;
    float c; // damping factor
    float k; // spring constant
}LevelCamera_t;

typedef struct Timer {
    float fractional;
    uint8_t seconds;
    uint8_t minutes;
} Timer_t;

typedef struct LevelSceneData {
    RenderTexture2D game_viewport;
    RenderTexture2D stat_view;
    RenderTexture2D credits_view;
    LevelCamera_t camera;
    Vector2 cam_pos;
    Rectangle game_rec;
    Vector2 game_field_size;
    GameState game_state;
    Image weapon_icons;
    float screenshake_time;
    Timer_t survival_timer;
    uint16_t endeffect_timer;
    Vector2 endeffect_pos;
    bool win_flag;
}LevelSceneData_t;

typedef struct LevelScene {
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_level_scene(LevelScene_t* scene);
void free_level_scene(LevelScene_t* scene);
#endif // SCENES_H
