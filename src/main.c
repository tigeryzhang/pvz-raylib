#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "presentation.h"
#include "pvz_config.h"
#include "raylib.h"
#include "raylib_frontend.h"

static const int CONFIGURED_ROWS = 4;
static const int CONFIGURED_COLS = 7;

static const int CONFIGURED_BOARD_RESOLUTION_WIDTH = 64;
static const int CONFIGURED_BOARD_RESOLUTION_HEIGHT = 32;

static const int CONFIGURED_HUD_RESOLUTION_WIDTH = 480;
static const int CONFIGURED_HUD_RESOLUTION_HEIGHT = 320;

static void apply_startup_config(GameConfig *config) {
	if (!config) {
		return;
	}

	config->rows = CONFIGURED_ROWS;
	config->cols = CONFIGURED_COLS;
	config->board_x_resolution = CONFIGURED_BOARD_RESOLUTION_WIDTH;
	config->board_y_resolution = CONFIGURED_BOARD_RESOLUTION_HEIGHT;
	config->hud_x_resolution = CONFIGURED_HUD_RESOLUTION_WIDTH;
	config->hud_y_resolution = CONFIGURED_HUD_RESOLUTION_HEIGHT;
}

int main(int argc, char **argv) {
	GameConfig config = pvz_make_default_config();
	apply_startup_config(&config);

	AppContext app;
	app_init(&app, &config);

	RenderView view;
	RenderData data;
	render_view_init(&view, config.board_x_resolution, config.board_y_resolution, config.hud_x_resolution,
					 config.hud_y_resolution);
	render_data_init(&data);

	app_prerender(&app, &view, &data);
	bool skip_live_render = true;

	DisplaySettings display_settings = set_display_settings(&config, 88, 12, 480, 320);
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(display_settings.window_width, display_settings.window_height, "pvz-raylib mockup");
	SetTargetFPS(PVZ_DEFAULT_TARGET_FPS);

	while (!WindowShouldClose() && !app.quit_requested) {
		InputFrame input;
		input_frame_reset(&input);
		raylib_poll_input(&app, &input);
		if (app_update(&app, &input, GetFrameTime()) == UPDATE_CHANGED_SCENE) {
			app_prerender(&app, &view, &data);
			skip_live_render = true;
		}
		if (!skip_live_render) {
			app_render(&app, &view, &data);
		} else {
			skip_live_render = false;
		}

		BeginDrawing();
		raylib_render_view(&app, &view);
		EndDrawing();
	}

	app_shutdown(&app);
	raylib_frontend_shutdown();
	CloseWindow();
	return 0;
}
