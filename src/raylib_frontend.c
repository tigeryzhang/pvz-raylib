#include "raylib_frontend.h"

#include <stdio.h>

#include "presentation.h"
#include "raylib.h"

typedef struct {
	RenderTexture2D board_target;
	int width;
	int height;
	bool loaded;
} FrontendState;

static FrontendState frontend_state = {0};

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
	default:
		return WHITE;
	}
}

static void ensure_board_target(const AppContext *app) {
	const int width = app->config.board_x_resolution;
	const int height = app->config.board_y_resolution;

	if (width <= 0 || height <= 0) {
		return;
	}

	if (frontend_state.loaded && frontend_state.width == width && frontend_state.height == height) {
		return;
	}

	if (frontend_state.loaded) {
		UnloadRenderTexture(frontend_state.board_target);
		frontend_state = (FrontendState){0};
	}

	frontend_state.board_target = LoadRenderTexture(width, height);
	SetTextureFilter(frontend_state.board_target.texture, TEXTURE_FILTER_POINT);
	frontend_state.width = width;
	frontend_state.height = height;
	frontend_state.loaded = true;
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

static void draw_char_3x5(char c, int x, int y, int scale, Color color) {
	const uint8_t *rows = glyph_rows_for_char(c);
	for (int row = 0; row < 5; ++row) {
		for (int col = 0; col < 3; ++col) {
			const int bit = 1 << (2 - col);
			if ((rows[row] & bit) == 0) {
				continue;
			}
			DrawRectangle(x + col * scale, y + row * scale, scale, scale, color);
		}
	}
}

static void draw_text_3x5(const char *text, int x, int y, int scale, Color color) {
	if (!text || scale <= 0) {
		return;
	}

	for (int i = 0; text[i] != '\0'; ++i) {
		draw_char_3x5(text[i], x + i * scale * 4, y, scale, color);
	}
}

static int text_width_3x5(const char *text, int scale) {
	int count = 0;
	while (text && text[count] != '\0') {
		++count;
	}
	return count > 0 ? count * scale * 4 - scale : 0;
}

static BoardCoord screen_to_board(const AppContext *app, int x, int y) {
	BoardCoord coord = {-1, -1};
	const IntRect board = app->display_settings.board_rect;
	if (!pvz_rect_contains(board, x, y)) {
		return coord;
	}

	coord.col = (x - board.x) * app->config.cols / board.w;
	coord.row = (y - board.y) * app->config.rows / board.h;
	return coord;
}

static RenderPalette plant_card_palette(PlantType type) {
	return type == PLANT_WALLNUT ? RENDER_PALETTE_WALLNUT : RENDER_PALETTE_PLANT;
}

static const char *plant_name(PlantType type) {
	switch (type) {
	case PLANT_SUNFLOWER:
		return "SUN";
	case PLANT_PEASHOOTER:
		return "PEA";
	case PLANT_WALLNUT:
		return "NUT";
	case PLANT_NONE:
	default:
		return "NONE";
	}
}

static int plant_cost(const AppContext *app, PlantType type) {
	switch (type) {
	case PLANT_SUNFLOWER:
		return app->config.sunflower_cost;
	case PLANT_PEASHOOTER:
		return app->config.peashooter_cost;
	case PLANT_WALLNUT:
		return app->config.wallnut_cost;
	case PLANT_NONE:
	default:
		return 0;
	}
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

static void draw_framebuffer_to_target(const RenderView *view) {
	for (int y = 0; y < view->board_height; ++y) {
		for (int x = 0; x < view->board_width; ++x) {
			DrawPixel(x, y, palette_color(view->board_pixels[y * view->board_width + x]));
		}
	}
}

static void draw_card(const AppContext *app, const RenderView *view, int index, PlantType type) {
	char buffer[16];
	const IntRect rect = pvz_get_card_rect(&app->display_settings, index);
	const bool selected = view->selected_plant == type;
	const Color fill = palette_color(selected ? RENDER_PALETTE_HIGHLIGHT : RENDER_PALETTE_TILE_DARK);
	const Color outline = palette_color(RENDER_PALETTE_TEXT);
	const Color icon = palette_color(plant_card_palette(type));

	DrawRectangle(rect.x, rect.y, rect.w, rect.h, fill);
	DrawRectangleLinesEx((Rectangle){(float)rect.x, (float)rect.y, (float)rect.w, (float)rect.h},
						 selected ? 4.0f : 2.0f, outline);
	DrawRectangle(rect.x + 12, rect.y + 12, 28, 28, icon);

	// Plant index
	snprintf(buffer, sizeof(buffer), "%d", index + 1);
	draw_text_3x5(buffer, rect.x + rect.w - 28, rect.y + 12, 4, outline);
	draw_text_3x5(plant_name(type), rect.x + 12, rect.y + 52, 3, outline);

	// Plant cost
	snprintf(buffer, sizeof(buffer), "%d", plant_cost(app, type));
	draw_text_3x5(buffer, rect.x + 12, rect.y + rect.h - 28, 3, palette_color(RENDER_PALETTE_SUN));
}

static void draw_play_hud(const AppContext *app, const RenderView *view) {
	char buffer[32];
	const IntRect hud = app->display_settings.hud_rect;
	const IntRect footer = app->display_settings.footer_rect;

	DrawRectangle(hud.x, hud.y, hud.w, hud.h, palette_color(RENDER_PALETTE_PANEL));
	DrawRectangle(footer.x, footer.y, footer.w, footer.h, palette_color(RENDER_PALETTE_PANEL));

	draw_card(app, view, 0, PLANT_SUNFLOWER);
	draw_card(app, view, 1, PLANT_PEASHOOTER);
	draw_card(app, view, 2, PLANT_WALLNUT);

	// Sun
	snprintf(buffer, sizeof(buffer), "SUN %d", view->sun_count);
	int sun_text_width = text_width_3x5(buffer, 3);
	draw_text_3x5(buffer, footer.x + footer.w - 14 - sun_text_width, footer.y + 14, 3,
				  palette_color(RENDER_PALETTE_TEXT));

	// Info
	draw_text_3x5("SPACE PAUSE", footer.x + 14, footer.y + 14, 3, palette_color(RENDER_PALETTE_TEXT));
	draw_text_3x5("R RESET", footer.x + 14, footer.y + 42, 3, palette_color(RENDER_PALETTE_TEXT));

	if (view->status != RENDER_STATUS_NONE && view->status != RENDER_STATUS_PLACEHOLDER && !view->paused &&
		view->game_status == GAME_STATUS_RUNNING) {
		const char *text = status_text(view);
		if (text) {
			const int width = text_width_3x5(text, 3);
			draw_text_3x5(text, footer.x + footer.w - width - 14, footer.y + 42, 3,
						  palette_color(status_palette(view)));
		}
	}
}

static void draw_placeholder_hud(const AppContext *app) {
	const IntRect hud = app->display_settings.hud_rect;
	const IntRect footer = app->display_settings.footer_rect;

	DrawRectangle(hud.x, hud.y, hud.w, hud.h, palette_color(RENDER_PALETTE_PANEL));
	DrawRectangle(footer.x, footer.y, footer.w, footer.h, palette_color(RENDER_PALETTE_PANEL));
	draw_text_3x5("PLACEHOLDER", hud.x + 16, hud.y + 18, 4, palette_color(RENDER_PALETTE_TEXT));
	draw_text_3x5("F1 PLAY", footer.x + 14, footer.y + 26, 4, palette_color(RENDER_PALETTE_HIGHLIGHT));
}

void raylib_poll_input(const AppContext *app, InputFrame *input) {
	input_frame_reset(input);

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
		bool handled = false;
		// Handle input consumption for ui
		for (int index = 0; index < 3; ++index) {
			const IntRect card = pvz_get_card_rect(&app->display_settings, index);
			if (pvz_rect_contains(card, mouse_x, mouse_y)) {
				input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_SELECT_CARD, .index = index});
				handled = true;
				break;
			}
		}

		if (!handled) {
			const BoardCoord coord = screen_to_board(app, mouse_x, mouse_y);
			if (coord.row >= 0) {
				input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_PLACE_TILE, .coord = coord});
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
		const BoardCoord coord = screen_to_board(app, mouse_x, mouse_y);
		if (coord.row >= 0) {
			input_frame_push(input, (InputCommand){.type = INPUT_COMMAND_REMOVE_TILE, .coord = coord});
		}
	}
}

void raylib_render_view(AppContext *app, const RenderView *view) {
	ensure_board_target(app);
	ClearBackground(palette_color(RENDER_PALETTE_BG));

	if (frontend_state.loaded) {
		BeginTextureMode(frontend_state.board_target);
		ClearBackground(palette_color(RENDER_PALETTE_BG));
		draw_framebuffer_to_target(view);
		EndTextureMode();

		const IntRect board = app->display_settings.board_rect;
		const Rectangle source = {0.0f, 0.0f, (float)frontend_state.width, (float)-frontend_state.height};
		const Rectangle dest = {(float)board.x, (float)board.y, (float)board.w, (float)board.h};
		DrawTexturePro(frontend_state.board_target.texture, source, dest, (Vector2){0}, 0.0f, WHITE);
	}

	if (view->status == RENDER_STATUS_PLACEHOLDER) {
		draw_placeholder_hud(app);
	} else {
		draw_play_hud(app, view);
	}
}

void raylib_frontend_shutdown(void) {
	if (!frontend_state.loaded) {
		return;
	}

	UnloadRenderTexture(frontend_state.board_target);
	frontend_state = (FrontendState){0};
}
