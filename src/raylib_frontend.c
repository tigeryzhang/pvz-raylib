#include "raylib_frontend.h"

#include <stdint.h>
#include <stdio.h>

#include "presentation.h"
#include "raylib.h"

typedef struct {
	DisplaySettings display_settings;
	RenderTexture2D board_target;
	int board_width;
	int board_height;
	RenderTexture2D hud_target;
	int hud_width;
	int hud_height;
	bool loaded;
} FrontendState;

static FrontendState frontend_state = {0};

DisplaySettings set_display_settings(const GameConfig *config, int tile_size, int margin, int hud_width,
									 int hud_height) {
	DisplaySettings display_settings = {0};

	display_settings.board_rect.x = margin;
	display_settings.board_rect.y = margin;
	display_settings.board_rect.w = config->cols * tile_size;
	display_settings.board_rect.h = config->rows * tile_size;

	display_settings.hud_rect.x = margin;
	display_settings.hud_rect.y = margin + display_settings.board_rect.h + margin;
	display_settings.hud_rect.w = hud_width;
	display_settings.hud_rect.h = hud_height;

	display_settings.window_width = display_settings.board_rect.w + margin * 2;
	display_settings.window_height = display_settings.board_rect.h + display_settings.hud_rect.h + margin * 3;

	frontend_state.display_settings = display_settings;

	return display_settings;
}

static Color palette_color(RenderPalette palette) {
	switch (palette) {
	case RENDER_PALETTE_BG:
		return (Color){26, 43, 26, 255};
	case RENDER_PALETTE_PANEL:
		return (Color){233, 223, 187, 255};
	case RENDER_PALETTE_TILE_LIGHT:
		return (Color){149, 196, 82, 255};
	case RENDER_PALETTE_TILE_DARK:
		return (Color){122, 168, 62, 255};
	case RENDER_PALETTE_HIGHLIGHT:
		return (Color){255, 208, 78, 255};
	case RENDER_PALETTE_TEXT:
		return (Color){49, 46, 37, 255};
	case RENDER_PALETTE_SUN:
		return (Color){255, 198, 48, 255};
	case RENDER_PALETTE_PLANT:
		return (Color){42, 137, 59, 255};
	case RENDER_PALETTE_WALLNUT:
		return (Color){141, 91, 46, 255};
	case RENDER_PALETTE_ZOMBIE:
		return (Color){105, 118, 110, 255};
	case RENDER_PALETTE_CONE:
		return (Color){235, 126, 34, 255};
	case RENDER_PALETTE_BUCKET:
		return (Color){137, 149, 160, 255};
	case RENDER_PALETTE_PROJECTILE:
		return (Color){80, 206, 68, 255};
	case RENDER_PALETTE_WARNING:
		return (Color){203, 72, 56, 255};
	case RENDER_PALETTE_SUCCESS:
		return (Color){65, 170, 95, 255};
	case RENDER_PALETTE_ART_0:
		return (Color){152, 170, 148, 255};
	case RENDER_PALETTE_ART_1:
		return (Color){64, 74, 66, 255};
	case RENDER_PALETTE_ART_2:
		return (Color){128, 93, 62, 255};
	case RENDER_PALETTE_ART_3:
		return (Color){84, 109, 138, 255};
	case RENDER_PALETTE_ART_4:
		return (Color){120, 40, 40, 255};
	case RENDER_PALETTE_ART_5:
		return (Color){61, 171, 87, 255};
	case RENDER_PALETTE_ART_6:
		return (Color){255, 161, 62, 255};
	case RENDER_PALETTE_ART_7:
		return (Color){169, 219, 109, 255};
	case RENDER_PALETTE_ART_8:
		return (Color){176, 121, 72, 255};
	default:
		return WHITE;
	}
}

static void ensure_render_targets(const AppContext *app) {
	const int board_width = app->config.board_x_resolution;
	const int board_height = app->config.board_y_resolution;
	const int hud_width = app->config.hud_x_resolution;
	const int hud_height = app->config.hud_y_resolution;

	if (board_width <= 0 || board_height <= 0 || hud_width <= 0 || hud_height <= 0) {
		return;
	}

	if (frontend_state.loaded && frontend_state.board_width == board_width &&
		frontend_state.board_height == board_height && frontend_state.hud_width == hud_width &&
		frontend_state.hud_height == hud_height) {
		return;
	}

	if (frontend_state.loaded) {
		UnloadRenderTexture(frontend_state.board_target);
		UnloadRenderTexture(frontend_state.hud_target);
		frontend_state = (FrontendState){0};
	}

	frontend_state.board_target = LoadRenderTexture(board_width, board_height);
	frontend_state.hud_target = LoadRenderTexture(hud_width, hud_height);
	SetTextureFilter(frontend_state.board_target.texture, TEXTURE_FILTER_POINT);
	SetTextureFilter(frontend_state.hud_target.texture, TEXTURE_FILTER_POINT);
	frontend_state.board_width = board_width;
	frontend_state.board_height = board_height;
	frontend_state.hud_width = hud_width;
	frontend_state.hud_height = hud_height;
	frontend_state.loaded = true;
}

static BoardCoord screen_to_board(const AppContext *app, int x, int y) {
	BoardCoord coord = {-1, -1};
	const IntRect board = frontend_state.display_settings.board_rect;
	if (!pvz_rect_contains(board, x, y)) {
		return coord;
	}

	coord.col = (x - board.x) * app->config.cols / board.w;
	coord.row = (y - board.y) * app->config.rows / board.h;
	return coord;
}

static const char *status_text(const RenderView *view) {
	if (view->game_status == GAME_STATUS_WON) {
		return "WIN";
	}
	if (view->game_status == GAME_STATUS_LOST) {
		return "LOSE";
	}
	if (view->paused) {
		return "PAUSE";
	}

	switch (view->status) {
	case RENDER_STATUS_PLACED:
		return "PLACED";
	case RENDER_STATUS_REMOVED:
		return "REMOVED";
	case RENDER_STATUS_OCCUPIED:
		return "OCCUPIED";
	case RENDER_STATUS_OUT_OF_BOUNDS:
		return "OOB";
	case RENDER_STATUS_NO_SELECTION:
		return "SELECT";
	case RENDER_STATUS_NO_SUN:
		return "NO SUN";
	case RENDER_STATUS_NOT_FOUND:
		return "EMPTY";
	case RENDER_STATUS_RESET:
		return "RESET";
	case RENDER_STATUS_PLACEHOLDER:
		return "F1 PLAY";
	case RENDER_STATUS_NONE:
	default:
		return NULL;
	}
}

static RenderPalette status_palette(const RenderView *view) {
	if (view->game_status == GAME_STATUS_WON) {
		return RENDER_PALETTE_SUCCESS;
	}
	if (view->game_status == GAME_STATUS_LOST) {
		return RENDER_PALETTE_WARNING;
	}
	if (view->paused) {
		return RENDER_PALETTE_PANEL;
	}

	switch (view->status) {
	case RENDER_STATUS_PLACED:
	case RENDER_STATUS_REMOVED:
	case RENDER_STATUS_RESET:
		return RENDER_PALETTE_SUCCESS;
	case RENDER_STATUS_OCCUPIED:
	case RENDER_STATUS_OUT_OF_BOUNDS:
	case RENDER_STATUS_NO_SELECTION:
	case RENDER_STATUS_NO_SUN:
	case RENDER_STATUS_NOT_FOUND:
		return RENDER_PALETTE_WARNING;
	case RENDER_STATUS_PLACEHOLDER:
		return RENDER_PALETTE_HIGHLIGHT;
	case RENDER_STATUS_NONE:
	default:
		return RENDER_PALETTE_TEXT;
	}
}

static void draw_framebuffer_to_target(const RenderView *view, RenderTarget target) {
	const uint8_t *pixels = target == RENDER_TARGET_BOARD ? view->board_pixels : view->hud_pixels;
	const int width = target == RENDER_TARGET_BOARD ? view->board_width : view->hud_width;
	const int height = target == RENDER_TARGET_BOARD ? view->board_height : view->hud_height;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			DrawPixel(x, y, palette_color(pixels[y * width + x]));
		}
	}
}

void raylib_poll_input(const AppContext *app, InputFrame *input) {
	if (IsKeyPressed(KEY_ONE)) {
		input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_SELECT_CARD, .index = 0});
	}
	if (IsKeyPressed(KEY_TWO)) {
		input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_SELECT_CARD, .index = 1});
	}
	if (IsKeyPressed(KEY_THREE)) {
		input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_SELECT_CARD, .index = 2});
	}
	if (IsKeyPressed(KEY_SPACE)) {
		input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_TOGGLE_PAUSE});
	}
	if (IsKeyPressed(KEY_R)) {
		input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_RESTART});
	}
	if (IsKeyPressed(KEY_F1)) {
		input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_GOTO_PLAY_SCENE});
	}
	if (IsKeyPressed(KEY_F2)) {
		input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_GOTO_PLACEHOLDER_SCENE});
	}

	const Vector2 mouse = GetMousePosition();
	const int mouse_x = (int)mouse.x;
	const int mouse_y = (int)mouse.y;

	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		// bool handled = false;
		// // Handle input consumption for ui
		// for (int index = 0; index < 3; ++index) {
		// 	const IntRect card = pvz_get_card_rect(&app->display_settings, index);
		// 	if (pvz_rect_contains(card, mouse_x, mouse_y)) {
		// 		input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_SELECT_CARD, .index = index});
		// 		handled = true;
		// 		break;
		// 	}
		// }

		// if (!handled) {
		const BoardCoord coord = screen_to_board(app, mouse_x, mouse_y);
		if (coord.row >= 0) {
			input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_PLACE_TILE, .coord = coord});
		}
		// }
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		const BoardCoord coord = screen_to_board(app, mouse_x, mouse_y);
		if (coord.row >= 0) {
			input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_REMOVE_TILE, .coord = coord});
		}
	}
}

void raylib_render_view(AppContext *app, RenderView *view) {
	ensure_render_targets(app);
	ClearBackground(palette_color(RENDER_PALETTE_BG));

	if (frontend_state.loaded) {
		// Update textures with framebuffers
		BeginTextureMode(frontend_state.board_target);
		ClearBackground(palette_color(RENDER_PALETTE_BG));
		draw_framebuffer_to_target(view, RENDER_TARGET_BOARD);
		EndTextureMode();

		BeginTextureMode(frontend_state.hud_target);
		ClearBackground(palette_color(RENDER_PALETTE_BG));
		draw_framebuffer_to_target(view, RENDER_TARGET_HUD);
		EndTextureMode();

		// Draw textures to screen
		const IntRect board_rect = frontend_state.display_settings.board_rect;
		const Rectangle board_src = {0.0f, 0.0f, (float)frontend_state.board_width,
									 (float)-frontend_state.board_height};
		const Rectangle board_dst = {(float)board_rect.x, (float)board_rect.y, (float)board_rect.w,
									 (float)board_rect.h};
		DrawTexturePro(frontend_state.board_target.texture, board_src, board_dst, (Vector2){0}, 0.0f, WHITE);

		const IntRect hud_rect = frontend_state.display_settings.hud_rect;
		const Rectangle hud_src = {0.0f, 0.0f, (float)frontend_state.hud_width, (float)-frontend_state.hud_height};
		const Rectangle hud_dst = {(float)hud_rect.x, (float)hud_rect.y, (float)hud_rect.w, (float)hud_rect.h};
		DrawTexturePro(frontend_state.hud_target.texture, hud_src, hud_dst, (Vector2){0}, 0.0f, WHITE);
	}

	// if (view->status == RENDER_STATUS_PLACEHOLDER) {
	// 	draw_placeholder_hud(app);
	// } else {
	// 	draw_play_hud(app, view);
	// }
}

void raylib_frontend_shutdown(void) {
	if (!frontend_state.loaded) {
		return;
	}

	UnloadRenderTexture(frontend_state.board_target);
	UnloadRenderTexture(frontend_state.hud_target);
	frontend_state = (FrontendState){0};
}
