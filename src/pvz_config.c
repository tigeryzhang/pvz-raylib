#include "pvz_config.h"
#include "pvz_utils.h"

GameConfig pvz_make_default_config(void) {
	GameConfig config = {
		.rows = 4,
		.cols = 7,
		.tile_size = 16,
		.margin = 12,
		.hud_width = 480,
		.hud_height = 320,
		.board_x_resolution = 128,
		.board_y_resolution = 64,
		.hud_x_resolution = 480,
		.hud_y_resolution = 320,
		.starting_sun = 200,
		.start_with_demo_layout = true,
		.sunflower_cost = 50,
		.sunflower_seed_cooldown = 2,
		.peashooter_cost = 100,
		.peashooter_seed_cooldown = 2,
		.wallnut_cost = 50,
		.wallnut_seed_cooldown = 5,
		.sunflower_health = 55,
		.peashooter_health = 70,
		.wallnut_health = 260,
		.pea_damage = 2,
		.zombie_health = 10,
		.cone_health = 10,
		.bucket_health = 10,
		.cone_armor = 10,
		.bucket_armor = 20,
		.zombie_bite_damage = 16,
		.fixed_dt = 1.0f / 30.0f,
		.zombie_spawn_interval = 2.7f,
		.zombie_attack_interval = 0.75f,
		.sunflower_sun_interval = 5.0f,
		.sunflower_collect_delay = 1.1f,
		.peashooter_fire_interval = 1.0f,
		.pea_speed_cells_per_second = 3.4f,
		.zombie_speed_regular = 0.22f,
		.zombie_speed_cone = 0.20f,
		.zombie_speed_bucket = 0.17f,
	};
	return config;
}

void pvz_clamp_config(GameConfig *config) {
	if (!config) {
		return;
	}

	config->rows = clamp_int(config->rows, 1, PVZ_MAX_ROWS);
	config->cols = clamp_int(config->cols, 3, PVZ_MAX_COLS);
	config->tile_size = clamp_int(config->tile_size, 32, 144);
	config->margin = clamp_int(config->margin, 8, 48);
	config->hud_width = clamp_int(config->hud_width, 1, PVZ_MAX_HUD_WIDTH);
	config->hud_height = clamp_int(config->hud_height, 1, PVZ_MAX_HUD_HEIGHT);
	config->board_x_resolution = clamp_int(config->board_x_resolution, config->cols, PVZ_MAX_BOARD_WIDTH);
	config->board_y_resolution = clamp_int(config->board_y_resolution, config->rows, PVZ_MAX_BOARD_HEIGHT);
	config->hud_x_resolution = clamp_int(config->hud_x_resolution, 1, PVZ_MAX_HUD_WIDTH);
	config->hud_y_resolution = clamp_int(config->hud_y_resolution, 1, PVZ_MAX_HUD_HEIGHT);
	config->sunflower_seed_cooldown = clamp_int(config->sunflower_seed_cooldown, 0, 600);
	config->peashooter_seed_cooldown = clamp_int(config->peashooter_seed_cooldown, 0, 600);
	config->wallnut_seed_cooldown = clamp_int(config->wallnut_seed_cooldown, 0, 600);

	if (config->fixed_dt <= 0.0f) {
		config->fixed_dt = 1.0f / 30.0f;
	}
}

DisplaySettings pvz_make_display_settings(const GameConfig *config) {
	DisplaySettings metrics = {0};

	metrics.board_rect.x = config->margin;
	metrics.board_rect.y = config->margin;
	metrics.board_rect.w = config->cols * config->tile_size;
	metrics.board_rect.h = config->rows * config->tile_size;

	metrics.hud_rect.x = config->margin;
	metrics.hud_rect.y = config->margin + metrics.board_rect.h + config->margin;
	metrics.hud_rect.w = config->hud_width;
	metrics.hud_rect.h = config->hud_height;

	metrics.window_width = metrics.board_rect.w + config->margin * 2;
	metrics.window_height = metrics.board_rect.h + metrics.hud_rect.h + config->margin * 3;

	return metrics;
}

bool pvz_rect_contains(IntRect rect, int x, int y) {
	return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}

int pvz_plant_cost(const GameConfig *config, PlantType type) {
	switch (type) {
	case PLANT_SUNFLOWER:
		return config->sunflower_cost;
	case PLANT_PEASHOOTER:
		return config->peashooter_cost;
	case PLANT_WALLNUT:
		return config->wallnut_cost;
	case PLANT_NONE:
	default:
		return 0;
	}
}

int pvz_plant_seed_cooldown(const GameConfig *config, PlantType type) {
	switch (type) {
	case PLANT_SUNFLOWER:
		return config->sunflower_seed_cooldown;
	case PLANT_PEASHOOTER:
		return config->peashooter_seed_cooldown;
	case PLANT_WALLNUT:
		return config->wallnut_seed_cooldown;
	case PLANT_NONE:
	default:
		return 0;
	}
}
