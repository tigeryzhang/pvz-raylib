#include "raylib_frontend.h"

#include "raylib.h"

typedef struct {
	RenderTexture2D board_target;
	int width;
	int height;
	bool loaded;
} FrontendState;

static FrontendState frontend_state = {0};

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
		return (Color){203, 72, 56, 185};
	case RENDER_PALETTE_SUCCESS:
		return (Color){65, 170, 95, 185};
	default:
		return WHITE;
	}
}

static void ensure_board_target(const AppContext *app) {
	const int width = app->config.board_resolution_width;
	const int height = app->config.board_resolution_height;
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

static void draw_rect(IntRect rect, Color color) { DrawRectangle(rect.x, rect.y, rect.w, rect.h, color); }

static void draw_card(IntRect rect, Color color, bool emphasized) {
	draw_rect(rect, color);
	const IntRect inner = {rect.x + 4, rect.y + 4, rect.w - 8, rect.h - 8};
	DrawRectangleLinesEx((Rectangle){(float)inner.x, (float)inner.y, (float)inner.w, (float)inner.h},
						 emphasized ? 4.0f : 2.0f, emphasized ? palette_color(RENDER_PALETTE_TEXT) : Fade(BLACK, 0.2f));
}

static void draw_sunflower(IntRect rect) {
	const int cx = rect.x + rect.w / 2;
	const int cy = rect.y + rect.h / 2;
	const int petal = rect.w / 5;
	for (int dx = -1; dx <= 1; ++dx) {
		for (int dy = -1; dy <= 1; ++dy) {
			if (dx == 0 && dy == 0) {
				continue;
			}
			DrawCircle(cx + dx * petal, cy + dy * petal, (float)petal * 0.55f, palette_color(RENDER_PALETTE_SUN));
		}
	}
	DrawCircle(cx, cy, (float)rect.w * 0.18f, (Color){88, 52, 21, 255});
	DrawRectangle(cx - rect.w / 12, cy, rect.w / 6, rect.h / 3, palette_color(RENDER_PALETTE_PLANT));
}

static void draw_peashooter(IntRect rect) {
	const Color stem = palette_color(RENDER_PALETTE_PLANT);
	DrawCircle(rect.x + rect.w / 2, rect.y + rect.h / 3, rect.w * 0.18f, stem);
	DrawCircle(rect.x + rect.w / 2 + rect.w / 5, rect.y + rect.h / 3, rect.w * 0.13f, stem);
	DrawCircle(rect.x + rect.w / 2 + rect.w / 3, rect.y + rect.h / 3, rect.w * 0.09f, palette_color(RENDER_PALETTE_BG));
	DrawRectangle(rect.x + rect.w / 2 - rect.w / 18, rect.y + rect.h / 3, rect.w / 9, rect.h / 3, stem);
	DrawCircle(rect.x + rect.w / 3, rect.y + rect.h * 3 / 4, rect.w * 0.12f, stem);
	DrawCircle(rect.x + rect.w * 2 / 3, rect.y + rect.h * 3 / 4, rect.w * 0.12f, stem);
}

static void draw_wallnut(IntRect rect) {
	const Color shell = palette_color(RENDER_PALETTE_WALLNUT);
	DrawEllipse(rect.x + rect.w / 2, rect.y + rect.h / 2, rect.w / 2.3f, rect.h / 2.2f, shell);
	DrawCircle(rect.x + rect.w / 2 - rect.w / 3, rect.y + rect.h / 2 - rect.h / 10, 1.5f, BLACK);
	DrawCircle(rect.x + rect.w / 2 + rect.w / 3, rect.y + rect.h / 2 - rect.h / 10, 1.5f, BLACK);
	DrawLine(rect.x + rect.w / 2 - rect.w / 7, rect.y + rect.h / 2 + rect.h / 9, rect.x + rect.w / 2 + rect.w / 7,
			 rect.y + rect.h / 2 + rect.h / 8, BLACK);
}

static void draw_regular_zombie(IntRect rect, RenderPalette palette) {
	const Color body = palette_color(palette);
	DrawCircle(rect.x + rect.w / 2, rect.y + rect.h / 4, rect.w * 0.18f, (Color){194, 203, 179, 255});
	DrawRectangle(rect.x + rect.w / 2 - rect.w / 10, rect.y + rect.h / 3, rect.w / 5, rect.h / 3, body);
	DrawRectangle(rect.x + rect.w / 2 - rect.w / 6, rect.y + rect.h * 2 / 3, rect.w / 8, rect.h / 4, body);
	DrawRectangle(rect.x + rect.w / 2 + rect.w / 18, rect.y + rect.h * 2 / 3, rect.w / 8, rect.h / 4, body);
	DrawRectangle(rect.x + rect.w / 2 - rect.w / 4, rect.y + rect.h / 3, rect.w / 8, rect.h / 4, body);
	DrawRectangle(rect.x + rect.w / 2 + rect.w / 8, rect.y + rect.h / 3, rect.w / 8, rect.h / 4, body);
}

static void draw_cone(IntRect rect) {
	draw_regular_zombie(rect, RENDER_PALETTE_ZOMBIE);
	Vector2 top = {(float)(rect.x + rect.w / 2), (float)(rect.y + 4)};
	Vector2 left = {(float)(rect.x + rect.w / 2 - rect.w / 6), (float)(rect.y + rect.h / 5)};
	Vector2 right = {(float)(rect.x + rect.w / 2 + rect.w / 6), (float)(rect.y + rect.h / 5)};
	DrawTriangle(top, left, right, palette_color(RENDER_PALETTE_CONE));
}

static void draw_bucket(IntRect rect) {
	draw_regular_zombie(rect, RENDER_PALETTE_ZOMBIE);
	draw_rect((IntRect){rect.x + rect.w / 2 - rect.w / 6, rect.y + 2, rect.w / 3, rect.h / 6},
			  palette_color(RENDER_PALETTE_BUCKET));
}

static void draw_circle(IntRect rect, Color color) {
	DrawCircle(rect.x + rect.w / 2, rect.y + rect.h / 2, rect.w / 2.0f, color);
}

static void draw_overlay(IntRect rect, RenderPalette palette) {
	Color color = palette_color(palette);
	color.a = 140;
	draw_rect(rect, color);
}

static void draw_item(const RenderItem *item) {
	switch (item->type) {
	case RENDER_ITEM_BACKGROUND:
	case RENDER_ITEM_PANEL:
	case RENDER_ITEM_TILE:
		draw_rect(item->rect, palette_color(item->palette));
		break;
	case RENDER_ITEM_CARD:
		draw_card(item->rect, palette_color(item->palette), item->emphasized);
		break;
	case RENDER_ITEM_SUNFLOWER:
		draw_sunflower(item->rect);
		break;
	case RENDER_ITEM_PEASHOOTER:
		draw_peashooter(item->rect);
		break;
	case RENDER_ITEM_WALLNUT:
		draw_wallnut(item->rect);
		break;
	case RENDER_ITEM_ZOMBIE_REGULAR:
		draw_regular_zombie(item->rect, item->palette);
		break;
	case RENDER_ITEM_ZOMBIE_CONE:
		draw_cone(item->rect);
		break;
	case RENDER_ITEM_ZOMBIE_BUCKET:
		draw_bucket(item->rect);
		break;
	case RENDER_ITEM_PROJECTILE:
	case RENDER_ITEM_SUN:
		draw_circle(item->rect, palette_color(item->palette));
		break;
	case RENDER_ITEM_OVERLAY:
		draw_overlay(item->rect, item->palette);
		break;
	case RENDER_ITEM_HIGHLIGHT:
	default:
		break;
	}
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

	for (int i = 0; i < view->item_count; ++i) {
		const RenderItem *item = &view->items[i];
		if (!item->board_space) {
			draw_item(item);
		}
	}

	if (frontend_state.loaded) {
		BeginTextureMode(frontend_state.board_target);
		ClearBackground(BLANK);
		for (int i = 0; i < view->item_count; ++i) {
			const RenderItem *item = &view->items[i];
			if (item->board_space) {
				draw_item(item);
			}
		}
		EndTextureMode();

		const IntRect board = app->display_settings.board_rect;
		const Rectangle source = {0.0f, 0.0f, (float)frontend_state.width, (float)-frontend_state.height};
		const Rectangle dest = {(float)board.x, (float)board.y, (float)board.w, (float)board.h};
		DrawTexturePro(frontend_state.board_target.texture, source, dest, (Vector2){0}, 0.0f, WHITE);
	}

	for (int i = 0; i < view->label_count; ++i) {
		const RenderLabel *label = &view->labels[i];
		DrawText(label->text, label->x, label->y, label->size, palette_color(label->palette));
	}
}

void raylib_frontend_shutdown(void) {
	if (!frontend_state.loaded) {
		return;
	}

	UnloadRenderTexture(frontend_state.board_target);
	frontend_state = (FrontendState){0};
}
