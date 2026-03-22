#include "presentation.h"
#include "pvz_config.h"
#include "pvz_utils.h"

#include <math.h>
#include <stdint.h>
#include <string.h>

static const uint8_t digit_glyphs[10][5] = {
	{7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1},
	{7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1}, {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
};

static const uint8_t letter_glyphs[26][5] = {
	{2, 5, 7, 5, 5}, {6, 5, 6, 5, 6}, {3, 4, 4, 4, 3}, {6, 5, 5, 5, 6}, {7, 4, 6, 4, 7}, {7, 4, 6, 4, 4},
	{3, 4, 5, 5, 3}, {5, 5, 7, 5, 5}, {7, 2, 2, 2, 7}, {1, 1, 1, 5, 2}, {5, 5, 6, 5, 5}, {4, 4, 4, 4, 7},
	{5, 7, 7, 5, 5}, {5, 7, 7, 7, 5}, {2, 5, 5, 5, 2}, {6, 5, 6, 4, 4}, {2, 5, 5, 7, 3}, {6, 5, 6, 5, 5},
	{3, 4, 2, 1, 6}, {7, 2, 2, 2, 2}, {5, 5, 5, 5, 7}, {5, 5, 5, 5, 2}, {5, 5, 7, 7, 5}, {5, 5, 2, 5, 5},
	{5, 5, 2, 2, 2}, {7, 1, 2, 4, 7},
};

static IntRect board_cell_rect(const GameState *game, int row, int col, int padding) {
	const int x0 = col * game->config->board_x_resolution / game->config->cols;
	const int x1 = (col + 1) * game->config->board_x_resolution / game->config->cols;
	const int y0 = row * game->config->board_y_resolution / game->config->rows;
	const int y1 = (row + 1) * game->config->board_y_resolution / game->config->rows;
	IntRect rect = {
		.x = x0,
		.y = y0,
		.w = x1 - x0,
		.h = y1 - y0,
	};
	if (padding > 0) {
		const int max_pad_x = rect.w > 1 ? (rect.w - 1) / 2 : 0;
		const int max_pad_y = rect.h > 1 ? (rect.h - 1) / 2 : 0;
		const int pad_x = padding < max_pad_x ? padding : max_pad_x;
		const int pad_y = padding < max_pad_y ? padding : max_pad_y;
		rect.x += pad_x;
		rect.y += pad_y;
		rect.w -= pad_x * 2;
		rect.h -= pad_y * 2;
	}
	return rect;
}

static IntRect board_entity_rect(const GameState *game, float row_center, float col_center, int size) {
	const int center_x = (int)lroundf(col_center * (float)game->config->board_x_resolution / (float)game->config->cols);
	const int center_y = (int)lroundf(row_center * (float)game->config->board_y_resolution / (float)game->config->rows);
	IntRect rect = {
		.x = center_x - size / 2,
		.y = center_y - size / 2,
		.w = size,
		.h = size,
	};
	return rect;
}

static int board_unit_size(const GameConfig *config) {
	const int cell_width = config->board_x_resolution / config->cols;
	const int cell_height = config->board_y_resolution / config->rows;
	return cell_width < cell_height ? cell_width : cell_height;
}

static int text_width_3x5(const char *text, int scale) {
	int count = 0;
	while (text && text[count] != '\0') {
		++count;
	}
	return count > 0 ? count * scale * 4 - scale : 0;
}

void render_view_reset(RenderView *view) {
	if (!view) {
		return;
	}
	memset(view, 0, sizeof(*view));
}

static void render_view_begin(RenderView *view, int board_width, int board_height, int hud_width, int hud_height,
							  RenderPalette clear_color) {
	if (!view) {
		return;
	}

	render_view_reset(view);

	view->board_width = clamp_int(board_width, 1, PVZ_MAX_BOARD_WIDTH);
	view->board_height = clamp_int(board_height, 1, PVZ_MAX_BOARD_HEIGHT);

	view->hud_width = clamp_int(hud_width, 1, PVZ_MAX_HUD_WIDTH);
	view->hud_height = clamp_int(hud_height, 1, PVZ_MAX_HUD_HEIGHT);

	memset(view->board_pixels, (unsigned char)clear_color, (size_t)view->board_width * (size_t)view->board_height);
	memset(view->hud_pixels, (unsigned char)clear_color, (size_t)view->hud_width * (size_t)view->hud_height);
}

static bool render_view_contains(const RenderView *view, RenderTarget target, int x, int y) {
	int w = target == RENDER_TARGET_BOARD ? view->board_width : view->hud_width;
	int h = target == RENDER_TARGET_BOARD ? view->board_height : view->hud_height;
	return view && x >= 0 && y >= 0 && x < w && y < h;
}

static uint8_t *get_pixels(RenderView *view, RenderTarget target) {
	switch (target) {
	case RENDER_TARGET_BOARD:
		return view->board_pixels;
	case RENDER_TARGET_HUD:
		return view->hud_pixels;
	}

	return NULL;
}

static void set_pixel(RenderView *view, RenderTarget target, int x, int y, RenderPalette palette) {
	if (!render_view_contains(view, target, x, y)) {
		return;
	}

	uint8_t *pixels = get_pixels(view, target);
	if (pixels == NULL) {
		return;
	}

	pixels[y * view->board_width + x] = (uint8_t)palette;
}

static void fill_rect(RenderView *view, RenderTarget target, IntRect rect, RenderPalette palette) {
	if (!view || rect.w <= 0 || rect.h <= 0) {
		return;
	}

	int width = target == RENDER_TARGET_BOARD ? view->board_width : view->hud_width;
	int height = target == RENDER_TARGET_BOARD ? view->board_height : view->hud_height;

	int x0 = rect.x;
	int y0 = rect.y;
	int x1 = rect.x + rect.w;
	int y1 = rect.y + rect.h;

	if (x0 < 0) {
		x0 = 0;
	}
	if (y0 < 0) {
		y0 = 0;
	}
	if (x1 > width) {
		x1 = width;
	}
	if (y1 > height) {
		y1 = height;
	}

	uint8_t *pixels = get_pixels(view, target);
	if (pixels == NULL) {
		return;
	}

	for (int y = y0; y < y1; ++y) {
		for (int x = x0; x < x1; ++x) {
			pixels[y * width + x] = (uint8_t)palette;
		}
	}
}

static const uint8_t *glyph_rows_for_char(char c) {
	static const uint8_t space_glyph[5] = {0, 0, 0, 0, 0};
	static const uint8_t dash_glyph[5] = {0, 0, 7, 0, 0};
	static const uint8_t colon_glyph[5] = {0, 2, 0, 2, 0};

	if (c >= 'a' && c <= 'z') {
		c = (char)(c - 'a' + 'A');
	}
	if (c >= '0' && c <= '9') {
		return digit_glyphs[c - '0'];
	}
	if (c >= 'A' && c <= 'Z') {
		return letter_glyphs[c - 'A'];
	}
	if (c == '-') {
		return dash_glyph;
	}
	if (c == ':') {
		return colon_glyph;
	}
	return space_glyph;
}

static void draw_char_3x5(RenderView *view, RenderTarget target, char c, int x, int y, int scale,
						  RenderPalette palette) {
	const uint8_t *rows = glyph_rows_for_char(c);
	for (int row = 0; row < 5; ++row) {
		for (int col = 0; col < 3; ++col) {
			const int bit = 1 << (2 - col);
			if ((rows[row] & bit) == 0) {
				continue;
			}
			fill_rect(view, target, (IntRect){.x = x + col * scale, .y = y + row * scale, .w = scale, .h = scale},
					  palette);
			// DrawRectangle(x + col * scale, y + row * scale, scale, scale, color);
		}
	}
}

static void draw_text_3x5(RenderView *view, RenderTarget target, const char *text, int x, int y, int scale,
						  RenderPalette palette) {
	if (!text || scale <= 0) {
		return;
	}

	for (int i = 0; text[i] != '\0'; ++i) {
		draw_char_3x5(view, target, text[i], x + i * scale * 4, y, scale, palette);
	}
}

static void draw_tile_checkerboard(RenderView *view, const GameState *game) {
	for (int row = 0; row < game->config->rows; ++row) {
		for (int col = 0; col < game->config->cols; ++col) {
			const IntRect rect = board_cell_rect(game, row, col, 1);
			const RenderPalette palette = ((row + col) % 2 == 0) ? RENDER_PALETTE_TILE_LIGHT : RENDER_PALETTE_TILE_DARK;
			fill_rect(view, RENDER_TARGET_BOARD, rect, palette);
		}
	}
}

static void draw_sunflower(RenderView *view, IntRect rect) {
	const int stem_w = rect.w / 6 > 0 ? rect.w / 6 : 1;
	const int center = rect.x + rect.w / 2;
	const int petal = rect.w / 5 > 0 ? rect.w / 5 : 1;
	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){center - stem_w / 2, rect.y + rect.h / 2, stem_w, rect.h / 3},
			  RENDER_PALETTE_PLANT);
	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){center - petal / 2, rect.y + rect.h / 2 - petal / 2, petal, petal},
			  RENDER_PALETTE_WALLNUT);
	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){center - petal / 2, rect.y + rect.h / 4 - petal / 2, petal, petal},
			  RENDER_PALETTE_SUN);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){center - petal / 2, rect.y + rect.h * 3 / 4 - petal / 2, petal, petal}, RENDER_PALETTE_SUN);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){rect.x + rect.w / 4 - petal / 2, rect.y + rect.h / 2 - petal / 2, petal, petal},
			  RENDER_PALETTE_SUN);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){rect.x + rect.w * 3 / 4 - petal / 2, rect.y + rect.h / 2 - petal / 2, petal, petal},
			  RENDER_PALETTE_SUN);
}

static void draw_peashooter(RenderView *view, IntRect rect) {
	const int body = rect.w / 4 > 0 ? rect.w / 4 : 1;
	const int center = rect.x + rect.w / 2;
	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){center - body / 2, rect.y + rect.h / 3, body, rect.h / 3},
			  RENDER_PALETTE_PLANT);
	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){center, rect.y + rect.h / 4, rect.w / 3, rect.h / 4},
			  RENDER_PALETTE_PLANT);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){center + rect.w / 4, rect.y + rect.h / 3, rect.w / 10 > 0 ? rect.w / 10 : 1,
						rect.h / 10 > 0 ? rect.h / 10 : 1},
			  RENDER_PALETTE_BG);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){center - rect.w / 4, rect.y + rect.h * 3 / 4, rect.w / 6 > 0 ? rect.w / 6 : 1,
						rect.h / 8 > 0 ? rect.h / 8 : 1},
			  RENDER_PALETTE_PLANT);
	fill_rect(
		view, RENDER_TARGET_BOARD,
		(IntRect){center, rect.y + rect.h * 3 / 4, rect.w / 6 > 0 ? rect.w / 6 : 1, rect.h / 8 > 0 ? rect.h / 8 : 1},
		RENDER_PALETTE_PLANT);
}

static void draw_wallnut(RenderView *view, IntRect rect) {
	fill_rect(view, RENDER_TARGET_BOARD, rect, RENDER_PALETTE_WALLNUT);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){rect.x + rect.w / 4, rect.y + rect.h / 3, rect.w / 10 > 0 ? rect.w / 10 : 1,
						rect.h / 10 > 0 ? rect.h / 10 : 1},
			  RENDER_PALETTE_TEXT);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){rect.x + rect.w * 3 / 5, rect.y + rect.h / 3, rect.w / 10 > 0 ? rect.w / 10 : 1,
						rect.h / 10 > 0 ? rect.h / 10 : 1},
			  RENDER_PALETTE_TEXT);
}

static void draw_plant(RenderView *view, PlantType type, IntRect rect) {
	switch (type) {
	case PLANT_SUNFLOWER:
		draw_sunflower(view, rect);
		break;
	case PLANT_PEASHOOTER:
		draw_peashooter(view, rect);
		break;
	case PLANT_WALLNUT:
		draw_wallnut(view, rect);
		break;
	case PLANT_NONE:
	default:
		break;
	}
}

static void draw_zombie(RenderView *view, ZombieType type, IntRect rect) {
	const int head = rect.h / 4 > 0 ? rect.h / 4 : 1;
	const int body_w = rect.w / 3 > 0 ? rect.w / 3 : 1;
	const int body_x = rect.x + rect.w / 2 - body_w / 2;

	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){body_x, rect.y + head, body_w, rect.h / 3}, RENDER_PALETTE_ZOMBIE);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){body_x - body_w / 2, rect.y + head + rect.h / 12, body_w / 2, rect.h / 4},
			  RENDER_PALETTE_ZOMBIE);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){body_x + body_w, rect.y + head + rect.h / 12, body_w / 2, rect.h / 4}, RENDER_PALETTE_ZOMBIE);
	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){body_x, rect.y + head + rect.h / 3, body_w / 3, rect.h / 3},
			  RENDER_PALETTE_ZOMBIE);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){body_x + body_w * 2 / 3, rect.y + head + rect.h / 3, body_w / 3, rect.h / 3},
			  RENDER_PALETTE_ZOMBIE);
	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){body_x, rect.y + rect.h / 12, body_w, head}, RENDER_PALETTE_TEXT);

	if (type == ZOMBIE_CONE) {
		fill_rect(view, RENDER_TARGET_BOARD, (IntRect){body_x - body_w / 4, rect.y, body_w + body_w / 2, head},
				  RENDER_PALETTE_CONE);
	} else if (type == ZOMBIE_BUCKETHEAD) {
		fill_rect(view, RENDER_TARGET_BOARD, (IntRect){body_x - body_w / 5, rect.y, body_w + body_w / 3, head},
				  RENDER_PALETTE_BUCKET);
	}
}

static void draw_projectile(RenderView *view, IntRect rect) {
	fill_rect(view, RENDER_TARGET_BOARD, rect, RENDER_PALETTE_PROJECTILE);
}

static void draw_sun(RenderView *view, IntRect rect) {
	fill_rect(view, RENDER_TARGET_BOARD, rect, RENDER_PALETTE_SUN);
	fill_rect(view, RENDER_TARGET_BOARD, (IntRect){rect.x + rect.w / 4, rect.y + rect.h / 4, rect.w / 2, rect.h / 2},
			  RENDER_PALETTE_HIGHLIGHT);
}

void presentation_build_play_view(RenderView *view, const GameState *game, RenderStatus status) {
	const int unit_size = board_unit_size(game->config);
	const int plant_padding = unit_size / 8 > 0 ? unit_size / 8 : 1;
	const int zombie_size = unit_size - unit_size / 5;
	const int projectile_size = unit_size / 5 > 0 ? unit_size / 5 : 1;
	const int sun_size = unit_size / 3 > 0 ? unit_size / 3 : 1;

	render_view_begin(view, game->config->board_x_resolution, game->config->board_y_resolution,
					  game->config->hud_x_resolution, game->config->hud_y_resolution, RENDER_PALETTE_BG);
	view->selected_plant = game->selected_plant;
	view->sun_count = game->sun_count;
	view->paused = game->paused;
	view->game_status = game->status;
	view->status = status;

	draw_tile_checkerboard(view, game);

	for (int i = 0; i < PVZ_MAX_PLANTS; ++i) {
		if (!game->plants[i].active) {
			continue;
		}
		const Plant *plant = &game->plants[i];
		draw_plant(view, plant->type, board_cell_rect(game, plant->coord.row, plant->coord.col, plant_padding));
	}

	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!game->zombies[i].active) {
			continue;
		}
		const Zombie *zombie = &game->zombies[i];
		draw_zombie(view, zombie->type,
					board_entity_rect(game, (float)zombie->lane + 0.5f, zombie->x + 0.5f, zombie_size));
	}

	for (int i = 0; i < PVZ_MAX_PROJECTILES; ++i) {
		if (!game->projectiles[i].active) {
			continue;
		}
		draw_projectile(view, board_entity_rect(game, (float)game->projectiles[i].lane + 0.5f,
												game->projectiles[i].x + 0.1f, projectile_size));
	}

	for (int i = 0; i < PVZ_MAX_SUNS; ++i) {
		if (!game->suns[i].active) {
			continue;
		}
		draw_sun(view, board_entity_rect(game, game->suns[i].y, game->suns[i].x, sun_size));
	}

	fill_rect(view, RENDER_TARGET_HUD, (IntRect){.x = 10, .y = 10, .w = 50, .h = 50}, RENDER_PALETTE_HIGHLIGHT);
	draw_text_3x5(view, RENDER_TARGET_HUD, "Test", 70, 10, 5, RENDER_PALETTE_TEXT);
}

void presentation_build_placeholder_view(RenderView *view, const GameConfig *config) {
	render_view_begin(view, config->board_x_resolution, config->board_y_resolution, config->hud_x_resolution,
					  config->hud_y_resolution, RENDER_PALETTE_BG);
	view->status = RENDER_STATUS_PLACEHOLDER;

	for (int y = 0; y < view->board_height; ++y) {
		for (int x = 0; x < view->board_width; ++x) {
			if (((x / 8) + (y / 8)) % 2 == 0) {
				set_pixel(view, RENDER_TARGET_BOARD, x, y, RENDER_PALETTE_TILE_DARK);
			} else {
				set_pixel(view, RENDER_TARGET_BOARD, x, y, RENDER_PALETTE_TILE_LIGHT);
			}
		}
	}

	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){view->board_width / 4, view->board_height / 4, view->board_width / 2, view->board_height / 2},
			  RENDER_PALETTE_PANEL);
	fill_rect(view, RENDER_TARGET_BOARD,
			  (IntRect){view->board_width / 4 + 4, view->board_height / 4 + 4, view->board_width / 2 - 8,
						view->board_height / 2 - 8},
			  RENDER_PALETTE_BG);
}
