#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "app.h"
#include "raylib.h"
#include "raylib_frontend.h"
#include "tests.h"

static const int CONFIGURED_ROWS = 4;
static const int CONFIGURED_COLS = 7;
static const int CONFIGURED_TILE_SIZE = 88;

static const int CONFIGURED_BOARD_RESOLUTION_WIDTH = 128;
static const int CONFIGURED_BOARD_RESOLUTION_HEIGHT = 64;

static void apply_startup_config(GameConfig *config) {
	if (!config) {
		return;
	}

	config->rows = CONFIGURED_ROWS;
	config->cols = CONFIGURED_COLS;
	config->tile_size = CONFIGURED_TILE_SIZE;
	config->board_resolution_width = CONFIGURED_BOARD_RESOLUTION_WIDTH;
	config->board_resolution_height = CONFIGURED_BOARD_RESOLUTION_HEIGHT;
}

static void print_usage(const char *argv0) { printf("Usage: %s [--test]\n", argv0); }

int main(int argc, char **argv) {
	GameConfig config = pvz_make_default_config();
	bool run_tests = false;
	apply_startup_config(&config);

	for (int i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--test") == 0) {
			run_tests = true;
			continue;
		}
		print_usage(argv[0]);
		return 1;
	}

	if (run_tests) {
		return run_all_tests();
	}

	AppContext app;
	app_init(&app, &config);
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(app.display_settings.window_width, app.display_settings.window_height, "pvz-raylib mockup");
	SetTargetFPS(60);
	app.render_view = raylib_render_view;

	while (!WindowShouldClose() && !app.quit_requested) {
		InputFrame input;
		RenderView view;
		raylib_poll_input(&app, &input);
		app_update(&app, &input, GetFrameTime());
		app_build_view(&app, &view);

		BeginDrawing();
		app_render(&app, &view);
		EndDrawing();
	}

	app_shutdown(&app);
	raylib_frontend_shutdown();
	CloseWindow();
	return 0;
}
