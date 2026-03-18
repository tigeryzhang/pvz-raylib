#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "pvz_config.h"

typedef enum {
	RENDER_PALETTE_BG = 0,
	RENDER_PALETTE_PANEL,
	RENDER_PALETTE_TILE_LIGHT,
	RENDER_PALETTE_TILE_DARK,
	RENDER_PALETTE_HIGHLIGHT,
	RENDER_PALETTE_TEXT,
	RENDER_PALETTE_SUN,
	RENDER_PALETTE_PLANT,
	RENDER_PALETTE_WALLNUT,
	RENDER_PALETTE_ZOMBIE,
	RENDER_PALETTE_CONE,
	RENDER_PALETTE_BUCKET,
	RENDER_PALETTE_PROJECTILE,
	RENDER_PALETTE_WARNING,
	RENDER_PALETTE_SUCCESS,
} RenderPalette;

typedef enum {
	RENDER_STATUS_NONE = 0,
	RENDER_STATUS_PLACED,
	RENDER_STATUS_REMOVED,
	RENDER_STATUS_OCCUPIED,
	RENDER_STATUS_OUT_OF_BOUNDS,
	RENDER_STATUS_NO_SELECTION,
	RENDER_STATUS_NO_SUN,
	RENDER_STATUS_NOT_FOUND,
	RENDER_STATUS_RESET,
	RENDER_STATUS_PLACEHOLDER,
} RenderStatus;

typedef struct {
	int board_width;
	int board_height;
	uint8_t pixels[PVZ_MAX_RENDER_PIXELS];
	PlantType selected_plant;
	int sun_count;
	bool paused;
	GameStatus game_status;
	RenderStatus status;
} RenderView;

void render_view_reset(RenderView *view);

// Specific impls for each game scene
void presentation_build_play_view(RenderView *view, const GameState *game, RenderStatus status);
void presentation_build_placeholder_view(RenderView *view, const GameConfig *config);
