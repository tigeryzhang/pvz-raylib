#include "tests.h"

#include <stdio.h>
#include <string.h>

#include "app.h"
#include "game.h"

typedef struct {
	int passed;
	int failed;
} TestStats;

static void record_assert(TestStats *stats, bool condition, const char *message) {
	if (condition) {
		stats->passed++;
		return;
	}

	stats->failed++;
	fprintf(stderr, "FAIL: %s\n", message);
}

static GameConfig make_test_config(void) {
	GameConfig config = pvz_make_default_config();
	config.start_with_demo_layout = false;
	config.rows = 4;
	config.cols = 7;
	config.fixed_dt = 0.05f;
	config.zombie_spawn_interval = 0.5f;
	config.sunflower_sun_interval = 0.25f;
	config.sunflower_collect_delay = 0.1f;
	config.peashooter_fire_interval = 0.1f;
	config.pea_speed_cells_per_second = 6.0f;
	config.zombie_speed_regular = 0.2f;
	config.zombie_speed_cone = 0.2f;
	config.zombie_speed_bucket = 0.2f;
	return config;
}

static void test_bounds_and_occupancy(TestStats *stats) {
	GameConfig config = make_test_config();
	GameState game;
	game_init(&game, &config);

	record_assert(stats, game_coord_in_bounds(&game, (BoardCoord){1, 1}), "expected in-bounds coordinate");
	record_assert(stats, !game_coord_in_bounds(&game, (BoardCoord){-1, 0}), "negative row should be invalid");
	record_assert(stats,
				  game_apply_command(&game,
									 (GameCommand){
										 .type = GAME_COMMAND_SELECT_PLANT,
										 .plant_type = PLANT_SUNFLOWER,
									 }) == GAME_COMMAND_RESULT_OK,
				  "select sunflower should succeed");
	record_assert(stats,
				  game_apply_command(&game,
									 (GameCommand){
										 .type = GAME_COMMAND_PLACE_PLANT,
										 .coord = {1, 1},
									 }) == GAME_COMMAND_RESULT_OK,
				  "placing sunflower should succeed");
	record_assert(stats,
				  game_apply_command(&game,
									 (GameCommand){
										 .type = GAME_COMMAND_PLACE_PLANT,
										 .coord = {1, 1},
									 }) == GAME_COMMAND_RESULT_OCCUPIED,
				  "placing twice on same tile should fail");
}

static void test_sunflower_generation(TestStats *stats) {
	GameConfig config = make_test_config();
	GameState game;
	game_init(&game, &config);
	const int before = game.sun_count;

	game_apply_command(&game, (GameCommand){.type = GAME_COMMAND_SELECT_PLANT, .plant_type = PLANT_SUNFLOWER});
	game_apply_command(&game, (GameCommand){.type = GAME_COMMAND_PLACE_PLANT, .coord = {0, 0}});

	for (int i = 0; i < 20; ++i) {
		game_step(&game, config.fixed_dt);
	}

	record_assert(stats, game.sun_count > before - config.sunflower_cost, "sunflower should generate sun income");
}

static void test_peashooter_combat(TestStats *stats) {
	GameConfig config = make_test_config();
	GameState game;
	game_init(&game, &config);

	game_apply_command(&game, (GameCommand){.type = GAME_COMMAND_SELECT_PLANT, .plant_type = PLANT_PEASHOOTER});
	game_apply_command(&game, (GameCommand){.type = GAME_COMMAND_PLACE_PLANT, .coord = {0, 0}});

	for (int i = 0; i < 120; ++i) {
		game_step(&game, config.fixed_dt);
		if (!game_has_active_zombies(&game) && game.wave_cursor > 0) {
			break;
		}
	}

	record_assert(stats, game.wave_cursor > 0, "scripted wave should spawn zombies");
	record_assert(stats, game_has_active_zombies(&game) || game.status == GAME_STATUS_WON,
				  "combat simulation should advance");
}

static void test_wallnut_blocking(TestStats *stats) {
	GameConfig config = make_test_config();
	GameState game;
	game_init(&game, &config);

	game_apply_command(&game, (GameCommand){.type = GAME_COMMAND_SELECT_PLANT, .plant_type = PLANT_WALLNUT});
	game_apply_command(&game, (GameCommand){.type = GAME_COMMAND_PLACE_PLANT, .coord = {0, 1}});

	for (int i = 0; i < 80; ++i) {
		game_step(&game, config.fixed_dt);
	}

	record_assert(stats, game.status == GAME_STATUS_RUNNING || game.status == GAME_STATUS_WON,
				  "wallnut should delay immediate loss");
}

static void test_zombie_differences(TestStats *stats) {
	GameConfig config = make_test_config();
	record_assert(stats, config.bucket_armor > config.cone_armor, "buckethead should have more armor than cone");
	record_assert(stats, config.cone_armor > 0, "cone should have armor");
}

static void test_scene_restart_and_transition(TestStats *stats) {
	GameConfig config = make_test_config();
	AppContext app;
	InputFrame input;
	RenderView view;
	app_init(&app, &config);

	input_frame_reset(&input);
	input_frame_push(&input, (InputCommand){.type = INPUT_COMMAND_SELECT_CARD, .index = 0});
	input_frame_push(&input, (InputCommand){.type = INPUT_COMMAND_PLACE_TILE, .coord = {2, 2}});
	app_update(&app, &input, config.fixed_dt);
	record_assert(stats, app.play_state.game.plant_grid[2][2] >= 0, "scene should place plant through input mapping");

	input_frame_reset(&input);
	input_frame_push(&input, (InputCommand){.type = INPUT_COMMAND_RESTART});
	app_update(&app, &input, config.fixed_dt);
	record_assert(stats, app.play_state.game.plant_grid[2][2] < 0, "restart should reset board state");

	input_frame_reset(&input);
	input_frame_push(&input, (InputCommand){.type = INPUT_COMMAND_GOTO_PLACEHOLDER_SCENE});
	app_update(&app, &input, config.fixed_dt);
	record_assert(stats, app.active_scene_id == SCENE_ID_PLACEHOLDER, "scene manager should transition to placeholder");

	app_build_view(&app, &view);
	record_assert(stats, view.status == RENDER_STATUS_PLACEHOLDER, "placeholder scene should build a placeholder view");
	app_shutdown(&app);
}

int run_all_tests(void) {
	TestStats stats = {0};
	test_bounds_and_occupancy(&stats);
	test_sunflower_generation(&stats);
	test_peashooter_combat(&stats);
	test_wallnut_blocking(&stats);
	test_zombie_differences(&stats);
	test_scene_restart_and_transition(&stats);

	printf("Tests passed: %d\n", stats.passed);
	if (stats.failed > 0) {
		printf("Tests failed: %d\n", stats.failed);
		return 1;
	}

	printf("All tests passed.\n");
	return 0;
}
