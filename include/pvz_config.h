// Needs the pvz prefix to avoid naming conflicts
#pragma once

#include <stdbool.h>

#define PVZ_MAX_ROWS 8
#define PVZ_MAX_COLS 12

#define PVZ_MAX_PLANTS 48
#define PVZ_MAX_ZOMBIES 32
#define PVZ_MAX_PROJECTILES 64
#define PVZ_MAX_SUNS 32

#define PVZ_MAX_BOARD_WIDTH 128
#define PVZ_MAX_BOARD_HEIGHT 64
#define PVZ_MAX_BOARD_PIXELS (PVZ_MAX_BOARD_WIDTH * PVZ_MAX_BOARD_HEIGHT)

#define PVZ_MAX_HUD_WIDTH 480
#define PVZ_MAX_HUD_HEIGHT 320
#define PVZ_MAX_HUD_PIXELS (PVZ_MAX_HUD_WIDTH * PVZ_MAX_HUD_HEIGHT)

#define PVZ_MAX_INPUT_COMMANDS 32

typedef struct {
	int x;
	int y;
	int w;
	int h;
} IntRect;

typedef struct {
	int rows;
	int cols;
	int tile_size;
	int board_x_resolution;
	int board_y_resolution;
	int hud_x_resolution;
	int hud_y_resolution;
	int margin;
	int hud_width;
	int hud_height;
	int starting_sun;
	int sunflower_cost;
	int peashooter_cost;
	int wallnut_cost;
	int sunflower_health;
	int peashooter_health;
	int wallnut_health;
	int zombie_health;
	int cone_health;
	int bucket_health;
	int cone_armor;
	int bucket_armor;
	int zombie_bite_damage;
	int pea_damage;
	float fixed_dt;
	float zombie_spawn_interval;
	float zombie_attack_interval;
	float sunflower_sun_interval;
	float sunflower_collect_delay;
	float peashooter_fire_interval;
	float pea_speed_cells_per_second;
	float zombie_speed_regular;
	float zombie_speed_cone;
	float zombie_speed_bucket;
	bool start_with_demo_layout;
} GameConfig;

typedef struct {
	int window_width;
	int window_height;
	IntRect hud_rect;
	IntRect board_rect;
} DisplaySettings;

GameConfig pvz_make_default_config(void);
void pvz_clamp_config(GameConfig *config);
DisplaySettings pvz_make_display_settings(const GameConfig *config);
IntRect pvz_get_card_rect(const DisplaySettings *metrics, int index);
bool pvz_rect_contains(IntRect rect, int x, int y);
