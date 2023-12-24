#ifndef SCENES_H
#define SCENES_H
#include "engine.h"
#include "ent_impl.h"

typedef struct UIButton {
    Rectangle box;
    bool pressed;
    bool hover;
    bool enabled;
} UIButton;

typedef struct StoreItem
{
    unsigned int cost;
    int8_t cap;
    int8_t remaining;
    unsigned int cost_increment;
    unsigned int cost_cap;
} StoreItem;

typedef struct UpgradeBox {
    UIButton button;
    int dot_spacing;
    bool show_dots;
    StoreItem* item;
} UpgradeBox;

typedef struct ShopUI {
    UpgradeBox upgrades[8];
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
} UpgradeStoreInventory;

typedef struct ShopSceneData {
    RenderTexture2D shop_viewport;
    Rectangle shop_rec;
    ShopUI ui;
    bool refresh_rec;
    UpgradeStoreInventory store;
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

typedef struct LevelSceneData {
    RenderTexture2D game_viewport;
    Camera2D cam;
    Rectangle game_rec;
    Vector2 game_field_size;
    GameState game_state;
}LevelSceneData_t;

typedef struct LevelScene {
    Scene_t scene;
    LevelSceneData_t data;
}LevelScene_t;

void init_level_scene(LevelScene_t* scene);
void free_level_scene(LevelScene_t* scene);
#endif // SCENES_H
