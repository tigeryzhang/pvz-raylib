#pragma once

#include "app.h"
#include "input.h"
#include "presentation.h"

typedef struct {
	int window_width;
	int window_height;
	IntRect hud_rect;
	IntRect board_rect;
} DisplaySettings;

DisplaySettings set_display_settings(const GameConfig *config, int tile_size, int margin, int hud_width,
									 int hud_height);

void raylib_poll_input(const AppContext *app, InputFrame *input);
void raylib_render_view(AppContext *app, RenderView *view);
void raylib_frontend_shutdown(void);
