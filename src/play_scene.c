#include "app.h"

#include <stdio.h>
#include <string.h>

static PlantType plant_from_card_index(int index) {
	switch (index) {
	case 0:
		return PLANT_SUNFLOWER;
	case 1:
		return PLANT_PEASHOOTER;
	case 2:
		return PLANT_WALLNUT;
	default:
		return PLANT_NONE;
	}
}

static const char *command_result_text(GameCommandResult result) {
	switch (result) {
	case GAME_COMMAND_RESULT_OK:
		return "Action applied";
	case GAME_COMMAND_RESULT_OCCUPIED:
		return "Tile already occupied";
	case GAME_COMMAND_RESULT_OUT_OF_BOUNDS:
		return "Tile outside board";
	case GAME_COMMAND_RESULT_NO_SELECTION:
		return "Select a plant first";
	case GAME_COMMAND_RESULT_NOT_ENOUGH_SUN:
		return "Not enough sun";
	case GAME_COMMAND_RESULT_NOT_FOUND:
		return "No plant on that tile";
	case GAME_COMMAND_RESULT_IGNORED:
	default:
		return "";
	}
}

static void play_scene_set_banner(PlaySceneState *state, const char *text, float duration) {
	snprintf(state->banner, sizeof(state->banner), "%s", text);
	state->banner_timer = duration;
}

static void play_scene_enter(Scene *scene, AppContext *app) {
	(void)scene;
	app->play_state.accumulator = 0.0f;
}

static void play_scene_handle_command(Scene *scene, AppContext *app, InputCommand command) {
	PlaySceneState *state = (PlaySceneState *)scene->state;

	switch (command.type) {
	case INPUT_COMMAND_SELECT_CARD: {
		GameCommandResult result =
			game_apply_command(&state->game, (GameCommand){
												 .type = GAME_COMMAND_SELECT_PLANT,
												 .plant_type = plant_from_card_index(command.index),
											 });
		if (result == GAME_COMMAND_RESULT_OK) {
			play_scene_set_banner(state, pvz_get_plant_name(state->game.selected_plant), 1.2f);
		}
		break;
	}
	case INPUT_COMMAND_PLACE_TILE: {
		GameCommandResult result = game_apply_command(&state->game, (GameCommand){
																		.type = GAME_COMMAND_PLACE_PLANT,
																		.coord = command.coord,
																	});
		const char *banner = result == GAME_COMMAND_RESULT_OK ? "Plant placed" : command_result_text(result);
		play_scene_set_banner(state, banner, 1.5f);
		break;
	}
	case INPUT_COMMAND_REMOVE_TILE: {
		GameCommandResult result = game_apply_command(&state->game, (GameCommand){
																		.type = GAME_COMMAND_REMOVE_PLANT,
																		.coord = command.coord,
																	});
		const char *banner = result == GAME_COMMAND_RESULT_OK ? "Plant removed" : command_result_text(result);
		play_scene_set_banner(state, banner, 1.5f);
		break;
	}
	case INPUT_COMMAND_TOGGLE_PAUSE:
		game_apply_command(&state->game, (GameCommand){.type = GAME_COMMAND_TOGGLE_PAUSE});
		play_scene_set_banner(state, state->game.paused ? "Paused" : "Running", 1.0f);
		break;
	case INPUT_COMMAND_RESTART:
		game_apply_command(&state->game, (GameCommand){.type = GAME_COMMAND_RESTART});
		play_scene_set_banner(state, "Board reset", 1.0f);
		break;
	case INPUT_COMMAND_GOTO_PLACEHOLDER_SCENE:
		scene_request(scene, SCENE_ID_PLACEHOLDER);
		break;
	case INPUT_COMMAND_GOTO_PLAY_SCENE:
	case INPUT_COMMAND_NONE:
	default:
		break;
	}
}

static void play_scene_update(Scene *scene, AppContext *app, const InputFrame *input, float frame_dt) {
	PlaySceneState *state = (PlaySceneState *)scene->state;

	for (int i = 0; i < input->count; ++i) {
		play_scene_handle_command(scene, app, input->commands[i]);
	}

	if (state->banner_timer > 0.0f) {
		state->banner_timer -= frame_dt;
		if (state->banner_timer <= 0.0f) {
			state->banner[0] = '\0';
		}
	}

	state->accumulator += frame_dt;
	while (state->accumulator >= app->config.fixed_dt) {
		game_step(&state->game, app->config.fixed_dt);
		state->accumulator -= app->config.fixed_dt;
	}
}

static void play_scene_build_view(Scene *scene, AppContext *app, RenderView *view) {
	PlaySceneState *state = (PlaySceneState *)scene->state;
	PlaySceneViewModel model = {};
	snprintf(model.banner, sizeof(model.banner), "%s", state->banner);
	presentation_build_play_view(view, &state->game, &app->display_settings, &model);
}

static void play_scene_render(Scene *scene, AppContext *app, const RenderView *view) {
	(void)scene;
	if (app->render_view) {
		app->render_view(app, view);
	}
}

static void play_scene_exit(Scene *scene, AppContext *app) {
	(void)scene;
	(void)app;
}

void play_scene_configure(Scene *scene, PlaySceneState *state) {
	static const SceneVTable vtable = {
		.enter = play_scene_enter,
		.update = play_scene_update,
		.build_view = play_scene_build_view,
		.render = play_scene_render,
		.exit = play_scene_exit,
	};

	scene->id = SCENE_ID_PLAY;
	scene->requested_scene = SCENE_ID_NONE;
	scene->state = state;
	scene->vtable = &vtable;
	memset(state, 0, sizeof(*state));
}
