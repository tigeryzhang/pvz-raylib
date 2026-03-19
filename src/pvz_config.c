#include "pvz_config.h"
#include "pvz_utils.h"

GameConfig pvz_make_default_config(void) {
	GameConfig config = {
		.rows = 4,
		.cols = 7,
		.tile_size = 16,
		.board_x_resolution = 128,
		.board_y_resolution = 64,
		.hud_x_resolution = 480,
		.hud_y_resolution = 320,
		.margin = 20,
		.hud_height = 120,
		.footer_height = 84,
		.starting_sun = 200,
		.sunflower_cost = 50,
		.peashooter_cost = 100,
		.wallnut_cost = 50,
		.sunflower_health = 55,
		.peashooter_health = 70,
		.wallnut_health = 260,
		.zombie_health = 10,
		.cone_health = 10,
		.bucket_health = 10,
		.cone_armor = 10,
		.bucket_armor = 20,
		.zombie_bite_damage = 16,
		.pea_damage = 2,
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
		.start_with_demo_layout = true,
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
	config->board_x_resolution = clamp_int(config->board_x_resolution, config->cols, 512);
	config->board_y_resolution = clamp_int(config->board_y_resolution, config->rows, 512);
	config->margin = clamp_int(config->margin, 8, 48);
	config->hud_height = clamp_int(config->hud_height, 72, 180);
	config->footer_height = clamp_int(config->footer_height, 48, 120);
	if (config->fixed_dt <= 0.0f) {
		config->fixed_dt = 1.0f / 30.0f;
	}
}

DisplaySettings pvz_make_display_settings(const GameConfig *config) {
	DisplaySettings metrics = {0};

	metrics.board_rect.x = config->margin;
	metrics.board_rect.y = config->margin + config->hud_height + config->margin;
	metrics.board_rect.w = config->cols * config->tile_size;
	metrics.board_rect.h = config->rows * config->tile_size;

	metrics.hud_rect.x = config->margin;
	metrics.hud_rect.y = config->margin;
	metrics.hud_rect.w = metrics.board_rect.w;
	metrics.hud_rect.h = config->hud_height;

	metrics.footer_rect.x = config->margin;
	metrics.footer_rect.y = metrics.board_rect.y + metrics.board_rect.h + config->margin;
	metrics.footer_rect.w = metrics.board_rect.w;
	metrics.footer_rect.h = config->footer_height;

	metrics.window_width = metrics.board_rect.w + config->margin * 2;
	metrics.window_height = metrics.footer_rect.y + metrics.footer_rect.h + config->margin;

	return metrics;
}

IntRect pvz_get_card_rect(const DisplaySettings *metrics, int index) {
	const int gap = 12;
	const int card_width = (metrics->hud_rect.w - gap * 4) / 3;
	IntRect rect = {
		.x = metrics->hud_rect.x + gap + index * (card_width + gap),
		.y = metrics->hud_rect.y + gap,
		.w = card_width,
		.h = metrics->hud_rect.h - gap * 2,
	};
	return rect;
}

bool pvz_rect_contains(IntRect rect, int x, int y) {
	return x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h;
}
