#pragma once

#include <stdbool.h>

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
	RENDER_ITEM_BACKGROUND = 0,
	RENDER_ITEM_PANEL,
	RENDER_ITEM_TILE,
	RENDER_ITEM_CARD,
	RENDER_ITEM_HIGHLIGHT,
	RENDER_ITEM_SUNFLOWER,
	RENDER_ITEM_PEASHOOTER,
	RENDER_ITEM_WALLNUT,
	RENDER_ITEM_ZOMBIE_REGULAR,
	RENDER_ITEM_ZOMBIE_CONE,
	RENDER_ITEM_ZOMBIE_BUCKET,
	RENDER_ITEM_PROJECTILE,
	RENDER_ITEM_SUN,
	RENDER_ITEM_OVERLAY,
} RenderItemType;

typedef struct {
	RenderItemType type;
	RenderPalette palette;
	IntRect rect;
	int value;
	bool board_space;
	bool emphasized;
} RenderItem;

typedef struct {
	char text[64];
	int x;
	int y;
	int size;
	RenderPalette palette;
} RenderLabel;

typedef struct {
	RenderItem items[PVZ_MAX_RENDER_ITEMS];
	int item_count;
	RenderLabel labels[PVZ_MAX_RENDER_LABELS];
	int label_count;
} RenderView;

typedef struct {
	char banner[64];
} PlaySceneViewModel;

void render_view_reset(RenderView *view);
bool render_view_push_item(RenderView *view, RenderItem item);
bool render_view_push_label(RenderView *view, const char *text, int x, int y, int size, RenderPalette palette);
void presentation_build_play_view(RenderView *view, const GameState *game, const DisplaySettings *metrics,
								  const PlaySceneViewModel *model);
void presentation_build_placeholder_view(RenderView *view, const DisplaySettings *metrics);
