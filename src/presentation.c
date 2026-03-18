#include "presentation.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool append_labelf(RenderView *view, int x, int y, int size, RenderPalette palette, const char *format, ...) {
	char buffer[64];
	va_list args;
	va_start(args, format);
	const int written = vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	if (written < 0) {
		return false;
	}
	return render_view_push_label(view, buffer, x, y, size, palette);
}

static IntRect board_cell_rect(const DisplaySettings *metrics, const GameState *game, int row, int col, int padding) {
	(void)metrics;
	const int x0 = col * game->config->board_resolution_width / game->config->cols;
	const int x1 = (col + 1) * game->config->board_resolution_width / game->config->cols;
	const int y0 = row * game->config->board_resolution_height / game->config->rows;
	const int y1 = (row + 1) * game->config->board_resolution_height / game->config->rows;
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

static IntRect board_entity_rect(const DisplaySettings *metrics, const GameState *game, float row_center,
								 float col_center, int size) {
	(void)metrics;
	const int center_x =
		(int)lroundf(col_center * (float)game->config->board_resolution_width / (float)game->config->cols);
	const int center_y =
		(int)lroundf(row_center * (float)game->config->board_resolution_height / (float)game->config->rows);
	IntRect rect = {
		.x = center_x - size / 2,
		.y = center_y - size / 2,
		.w = size,
		.h = size,
	};
	return rect;
}

static IntRect board_bounds_rect(const GameConfig *config) {
	return (IntRect){0, 0, config->board_resolution_width, config->board_resolution_height};
}

static int board_unit_size(const GameConfig *config) {
	const int cell_width = config->board_resolution_width / config->cols;
	const int cell_height = config->board_resolution_height / config->rows;
	return cell_width < cell_height ? cell_width : cell_height;
}

void render_view_reset(RenderView *view) {
	if (!view) {
		return;
	}
	memset(view, 0, sizeof(*view));
}

bool render_view_push_item(RenderView *view, RenderItem item) {
	if (!view || view->item_count >= PVZ_MAX_RENDER_ITEMS) {
		return false;
	}
	view->items[view->item_count++] = item;
	return true;
}

bool render_view_push_label(RenderView *view, const char *text, int x, int y, int size, RenderPalette palette) {
	if (!view || view->label_count >= PVZ_MAX_RENDER_LABELS) {
		return false;
	}

	RenderLabel *label = &view->labels[view->label_count++];
	snprintf(label->text, sizeof(label->text), "%s", text);
	label->x = x;
	label->y = y;
	label->size = size;
	label->palette = palette;
	return true;
}

static RenderItemType plant_glyph(PlantType plant_type) {
	switch (plant_type) {
	case PLANT_SUNFLOWER:
		return RENDER_ITEM_SUNFLOWER;
	case PLANT_PEASHOOTER:
		return RENDER_ITEM_PEASHOOTER;
	case PLANT_WALLNUT:
		return RENDER_ITEM_WALLNUT;
	case PLANT_NONE:
	default:
		return RENDER_ITEM_CARD;
	}
}

static RenderItemType zombie_glyph(ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_CONE:
		return RENDER_ITEM_ZOMBIE_CONE;
	case ZOMBIE_BUCKETHEAD:
		return RENDER_ITEM_ZOMBIE_BUCKET;
	case ZOMBIE_REGULAR:
	default:
		return RENDER_ITEM_ZOMBIE_REGULAR;
	}
}

void presentation_build_play_view(RenderView *view, const GameState *game, const DisplaySettings *metrics,
								  const PlaySceneViewModel *model) {
	render_view_reset(view);
	const int board_unit = board_unit_size(game->config);
	const int plant_padding = board_unit / 8 > 0 ? board_unit / 8 : 1;
	const int zombie_size = board_unit - board_unit / 5;
	const int projectile_size = board_unit / 5 > 0 ? board_unit / 5 : 1;
	const int sun_size = board_unit / 3 > 0 ? board_unit / 3 : 1;

	render_view_push_item(view, (RenderItem){
									.type = RENDER_ITEM_BACKGROUND,
									.palette = RENDER_PALETTE_BG,
									.rect = {0, 0, metrics->window_width, metrics->window_height},
								});
	render_view_push_item(view, (RenderItem){
									.type = RENDER_ITEM_PANEL,
									.palette = RENDER_PALETTE_PANEL,
									.rect = metrics->hud_rect,
								});
	render_view_push_item(view, (RenderItem){
									.type = RENDER_ITEM_PANEL,
									.palette = RENDER_PALETTE_PANEL,
									.rect = metrics->footer_rect,
								});

	for (int index = 0; index < 3; ++index) {
		const PlantType plant_type = (PlantType)(index + 1);
		const IntRect rect = pvz_get_card_rect(metrics, index);
		const bool selected = game->selected_plant == plant_type;
		render_view_push_item(view, (RenderItem){
										.type = RENDER_ITEM_CARD,
										.palette = selected ? RENDER_PALETTE_HIGHLIGHT : RENDER_PALETTE_TILE_DARK,
										.rect = rect,
										.emphasized = selected,
									});
		render_view_push_item(
			view, (RenderItem){
					  .type = plant_glyph(plant_type),
					  .palette = (plant_type == PLANT_WALLNUT) ? RENDER_PALETTE_WALLNUT : RENDER_PALETTE_PLANT,
					  .rect = {rect.x + 10, rect.y + 10, 56, 56},
				  });
		append_labelf(view, rect.x + 72, rect.y + 14, 18, RENDER_PALETTE_TEXT, "%d %s", index + 1,
					  pvz_get_plant_name(plant_type));
		append_labelf(view, rect.x + 72, rect.y + 42, 18, RENDER_PALETTE_SUN, "Cost %d",
					  game_get_plant_cost(game, plant_type));
	}

	for (int row = 0; row < game->config->rows; ++row) {
		for (int col = 0; col < game->config->cols; ++col) {
			render_view_push_item(
				view, (RenderItem){
						  .type = RENDER_ITEM_TILE,
						  .palette = ((row + col) % 2 == 0) ? RENDER_PALETTE_TILE_LIGHT : RENDER_PALETTE_TILE_DARK,
						  .rect = board_cell_rect(metrics, game, row, col, 1),
						  .board_space = true,
					  });
		}
	}

	for (int i = 0; i < PVZ_MAX_PLANTS; ++i) {
		if (!game->plants[i].active) {
			continue;
		}
		const Plant *plant = &game->plants[i];
		render_view_push_item(
			view,
			(RenderItem){
				.type = plant_glyph(plant->type),
				.palette = (plant->type == PLANT_WALLNUT) ? RENDER_PALETTE_WALLNUT : RENDER_PALETTE_PLANT,
				.rect = board_cell_rect(metrics, game, plant->coord.row, plant->coord.col, plant_padding),
				.value = plant->health,
				.board_space = true,
			});
	}

	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!game->zombies[i].active) {
			continue;
		}
		const Zombie *zombie = &game->zombies[i];
		render_view_push_item(view, (RenderItem){
										.type = zombie_glyph(zombie->type),
										.palette = (zombie->type == ZOMBIE_CONE)
													   ? RENDER_PALETTE_CONE
													   : (zombie->type == ZOMBIE_BUCKETHEAD ? RENDER_PALETTE_BUCKET
																							: RENDER_PALETTE_ZOMBIE),
										.rect = board_entity_rect(metrics, game, (float)zombie->lane + 0.5f,
																  zombie->x + 0.5f, zombie_size),
										.value = zombie->health + zombie->armor,
										.board_space = true,
									});
	}

	for (int i = 0; i < PVZ_MAX_PROJECTILES; ++i) {
		if (!game->projectiles[i].active) {
			continue;
		}
		render_view_push_item(view,
							  (RenderItem){
								  .type = RENDER_ITEM_PROJECTILE,
								  .palette = RENDER_PALETTE_PROJECTILE,
								  .rect = board_entity_rect(metrics, game, (float)game->projectiles[i].lane + 0.5f,
															game->projectiles[i].x + 0.1f, projectile_size),
								  .board_space = true,
							  });
	}

	for (int i = 0; i < PVZ_MAX_SUNS; ++i) {
		if (!game->suns[i].active) {
			continue;
		}
		render_view_push_item(view, (RenderItem){
										.type = RENDER_ITEM_SUN,
										.palette = RENDER_PALETTE_SUN,
										.rect = board_entity_rect(metrics, game, game->suns[i].y, game->suns[i].x,
																  sun_size),
										.board_space = true,
									});
	}

	append_labelf(view, metrics->hud_rect.x + 16, metrics->hud_rect.y + metrics->hud_rect.h - 28, 20,
				  RENDER_PALETTE_TEXT, "Sun %d", game->sun_count);
	append_labelf(view, metrics->footer_rect.x + 14, metrics->footer_rect.y + 12, 18, RENDER_PALETTE_TEXT,
				  "Selected %s | Space pause | R restart", pvz_get_plant_name(game->selected_plant));
	append_labelf(view, metrics->footer_rect.x + 14, metrics->footer_rect.y + 40, 18, RENDER_PALETTE_TEXT,
				  "F2 placeholder scene | Left click place | Right click remove");

	if (model->banner[0] != '\0') {
		render_view_push_label(view, model->banner, metrics->footer_rect.x + metrics->footer_rect.w - 340,
							   metrics->footer_rect.y + 12, 18, RENDER_PALETTE_WARNING);
	}

	if (game->paused) {
		render_view_push_item(view, (RenderItem){
										.type = RENDER_ITEM_OVERLAY,
										.palette = RENDER_PALETTE_PANEL,
										.rect = board_bounds_rect(game->config),
										.board_space = true,
									});
		render_view_push_label(view, "Paused", metrics->board_rect.x + metrics->board_rect.w / 2 - 48,
							   metrics->board_rect.y + metrics->board_rect.h / 2 - 14, 28, RENDER_PALETTE_TEXT);
	}

	if (game->status == GAME_STATUS_WON || game->status == GAME_STATUS_LOST) {
		const RenderPalette palette = game->status == GAME_STATUS_WON ? RENDER_PALETTE_SUCCESS : RENDER_PALETTE_WARNING;
		render_view_push_item(view, (RenderItem){
										.type = RENDER_ITEM_OVERLAY,
										.palette = palette,
										.rect = board_bounds_rect(game->config),
										.board_space = true,
									});
		render_view_push_label(view, game->status == GAME_STATUS_WON ? "Wave Cleared" : "House Breached",
							   metrics->board_rect.x + metrics->board_rect.w / 2 - 110,
							   metrics->board_rect.y + metrics->board_rect.h / 2 - 14, 30, RENDER_PALETTE_TEXT);
	}
}

void presentation_build_placeholder_view(RenderView *view, const DisplaySettings *metrics) {
	render_view_reset(view);

	render_view_push_item(view, (RenderItem){
									.type = RENDER_ITEM_BACKGROUND,
									.palette = RENDER_PALETTE_BG,
									.rect = {0, 0, metrics->window_width, metrics->window_height},
								});
	render_view_push_item(
		view, (RenderItem){
				  .type = RENDER_ITEM_PANEL,
				  .palette = RENDER_PALETTE_PANEL,
				  .rect = {metrics->hud_rect.x, metrics->hud_rect.y + 40, metrics->hud_rect.w, metrics->board_rect.h},
			  });
	render_view_push_label(view, "Placeholder Scene", metrics->hud_rect.x + 32, metrics->hud_rect.y + 72, 36,
						   RENDER_PALETTE_TEXT);
	render_view_push_label(view, "This is where a title or plant-selection scene can plug in later.",
						   metrics->hud_rect.x + 32, metrics->hud_rect.y + 128, 22, RENDER_PALETTE_TEXT);
	render_view_push_label(view, "Press F1 to return to the playable board.", metrics->hud_rect.x + 32,
						   metrics->hud_rect.y + 170, 22, RENDER_PALETTE_HIGHLIGHT);
}
