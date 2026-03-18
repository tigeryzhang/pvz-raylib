#include "game.h"

#include <math.h>
#include <string.h>

// TODO: Temp solution for a wave definition
static const ZombieType zombie_wave_types[] = {
	ZOMBIE_REGULAR,	   ZOMBIE_CONE,	   ZOMBIE_BUCKETHEAD, ZOMBIE_REGULAR,	 ZOMBIE_CONE,
	ZOMBIE_BUCKETHEAD, ZOMBIE_REGULAR, ZOMBIE_CONE,		  ZOMBIE_BUCKETHEAD,
};

static void clear_state_arrays(GameState *state) {
	memset(state->plants, 0, sizeof(state->plants));
	memset(state->zombies, 0, sizeof(state->zombies));
	memset(state->projectiles, 0, sizeof(state->projectiles));
	memset(state->suns, 0, sizeof(state->suns));
	for (int row = 0; row < PVZ_MAX_ROWS; ++row) {
		for (int col = 0; col < PVZ_MAX_COLS; ++col) {
			state->plant_grid[row][col] = -1;
		}
	}
}

static int zombie_total_count(void) { return (int)(sizeof(zombie_wave_types) / sizeof(zombie_wave_types[0])); }

static int get_free_plant_slot(GameState *state) {
	for (int i = 0; i < PVZ_MAX_PLANTS; ++i) {
		if (!state->plants[i].active) {
			return i;
		}
	}
	return -1;
}

static int get_free_zombie_slot(GameState *state) {
	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!state->zombies[i].active) {
			return i;
		}
	}
	return -1;
}

static int get_free_projectile_slot(GameState *state) {
	for (int i = 0; i < PVZ_MAX_PROJECTILES; ++i) {
		if (!state->projectiles[i].active) {
			return i;
		}
	}
	return -1;
}

static int get_free_sun_slot(GameState *state) {
	for (int i = 0; i < PVZ_MAX_SUNS; ++i) {
		if (!state->suns[i].active) {
			return i;
		}
	}
	return -1;
}

static int get_plant_health(const GameState *state, PlantType plant_type) {
	switch (plant_type) {
	case PLANT_SUNFLOWER:
		return state->config->sunflower_health;
	case PLANT_PEASHOOTER:
		return state->config->peashooter_health;
	case PLANT_WALLNUT:
		return state->config->wallnut_health;
	case PLANT_NONE:
	default:
		return 0;
	}
}

int game_get_plant_cost(const GameState *state, PlantType plant_type) {
	switch (plant_type) {
	case PLANT_SUNFLOWER:
		return state->config->sunflower_cost;
	case PLANT_PEASHOOTER:
		return state->config->peashooter_cost;
	case PLANT_WALLNUT:
		return state->config->wallnut_cost;
	case PLANT_NONE:
	default:
		return 0;
	}
}

static float get_zombie_speed(const GameState *state, ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_CONE:
		return state->config->zombie_speed_cone;
	case ZOMBIE_BUCKETHEAD:
		return state->config->zombie_speed_bucket;
	case ZOMBIE_REGULAR:
	default:
		return state->config->zombie_speed_regular;
	}
}

static int get_zombie_health(const GameState *state, ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_CONE:
		return state->config->cone_health;
	case ZOMBIE_BUCKETHEAD:
		return state->config->bucket_health;
	case ZOMBIE_REGULAR:
	default:
		return state->config->zombie_health;
	}
}

static int get_zombie_armor(const GameState *state, ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_CONE:
		return state->config->cone_armor;
	case ZOMBIE_BUCKETHEAD:
		return state->config->bucket_armor;
	case ZOMBIE_REGULAR:
	default:
		return 0;
	}
}

static void remove_plant(GameState *state, int index) {
	if (index < 0 || index >= PVZ_MAX_PLANTS || !state->plants[index].active) {
		return;
	}

	BoardCoord coord = state->plants[index].coord;
	state->plant_grid[coord.row][coord.col] = -1;
	state->plants[index].active = false;
}

static bool has_zombie_ahead_in_lane(const GameState *state, int lane, int col) {
	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!state->zombies[i].active || state->zombies[i].lane != lane) {
			continue;
		}
		if (state->zombies[i].x > (float)col + 0.2f) {
			return true;
		}
	}
	return false;
}

static void spawn_projectile(GameState *state, int lane, float x) {
	const int slot = get_free_projectile_slot(state);
	if (slot < 0) {
		return;
	}

	state->projectiles[slot].active = true;
	state->projectiles[slot].lane = lane;
	state->projectiles[slot].x = x;
	state->projectiles[slot].damage = state->config->pea_damage;
	state->projectiles[slot].speed = state->config->pea_speed_cells_per_second;
}

static void spawn_sun(GameState *state, BoardCoord coord) {
	const int slot = get_free_sun_slot(state);
	if (slot < 0) {
		return;
	}

	state->suns[slot].active = true;
	state->suns[slot].x = (float)coord.col + 0.5f;
	state->suns[slot].y = (float)coord.row + 0.5f;
	state->suns[slot].value = 25;
	state->suns[slot].collect_timer = state->config->sunflower_collect_delay;
}

static bool place_plant_internal(GameState *state, PlantType plant_type, BoardCoord coord, bool charge_cost) {
	const int slot = get_free_plant_slot(state);
	if (slot < 0) {
		return false;
	}

	const int cost = game_get_plant_cost(state, plant_type);
	if (charge_cost) {
		state->sun_count -= cost;
	}

	state->plants[slot].active = true;
	state->plants[slot].type = plant_type;
	state->plants[slot].coord = coord;
	state->plants[slot].health = get_plant_health(state, plant_type);
	// Start timer at halfway
	state->plants[slot].action_timer = (plant_type == PLANT_SUNFLOWER) ? state->config->sunflower_sun_interval * 0.5f
																	   : state->config->peashooter_fire_interval * 0.5f;
	state->plant_grid[coord.row][coord.col] = slot;
	return true;
}

static void spawn_demo_plants(GameState *state) {
	if (state->config->rows <= 0 || state->config->cols <= 0) {
		return;
	}

	BoardCoord sunflower = {0, 0};
	BoardCoord peashooter = {state->config->rows > 1 ? 1 : 0, state->config->cols > 1 ? 1 : 0};
	BoardCoord wallnut = {state->config->rows > 2 ? 2 : state->config->rows - 1,
						  state->config->cols > 2 ? 2 : state->config->cols - 1};
	place_plant_internal(state, PLANT_SUNFLOWER, sunflower, false);
	if (state->plant_grid[peashooter.row][peashooter.col] < 0) {
		place_plant_internal(state, PLANT_PEASHOOTER, peashooter, false);
	}
	if (state->plant_grid[wallnut.row][wallnut.col] < 0) {
		place_plant_internal(state, PLANT_WALLNUT, wallnut, false);
	}
}

static void spawn_next_zombie(GameState *state) {
	if (state->wave_cursor >= zombie_total_count()) {
		return;
	}

	const int slot = get_free_zombie_slot(state);
	if (slot < 0) {
		return;
	}

	const ZombieType type = zombie_wave_types[state->wave_cursor];
	const int lane = state->wave_cursor % state->config->rows;
	Zombie *zombie = &state->zombies[slot];
	zombie->active = true;
	zombie->type = type;
	zombie->lane = lane;
	zombie->x = (float)state->config->cols + 0.45f;
	zombie->health = get_zombie_health(state, type);
	zombie->armor = get_zombie_armor(state, type);
	zombie->speed = get_zombie_speed(state, type);
	zombie->attack_timer = state->config->zombie_attack_interval;
	state->wave_cursor++;
}

static void damage_zombie(Zombie *zombie, int damage) {
	if (zombie->armor > 0) {
		const int absorbed = damage < zombie->armor ? damage : zombie->armor;
		zombie->armor -= absorbed;
		damage -= absorbed;
	}
	if (damage > 0) {
		zombie->health -= damage;
	}
	if (zombie->health <= 0) {
		zombie->active = false;
	}
}

static void step_suns(GameState *state, float dt) {
	for (int i = 0; i < PVZ_MAX_SUNS; ++i) {
		if (!state->suns[i].active) {
			continue;
		}
		state->suns[i].collect_timer -= dt;
		if (state->suns[i].collect_timer <= 0.0f) {
			state->sun_count += state->suns[i].value;
			state->suns[i].active = false;
		}
	}
}

static void step_plants(GameState *state, float dt) {
	for (int i = 0; i < PVZ_MAX_PLANTS; ++i) {
		if (!state->plants[i].active) {
			continue;
		}

		Plant *plant = &state->plants[i];
		plant->action_timer -= dt;
		if (plant->type == PLANT_SUNFLOWER) {
			if (plant->action_timer <= 0.0f) {
				spawn_sun(state, plant->coord);
				plant->action_timer += state->config->sunflower_sun_interval;
			}
			continue;
		}

		if (plant->type == PLANT_PEASHOOTER && plant->action_timer <= 0.0f &&
			has_zombie_ahead_in_lane(state, plant->coord.row, plant->coord.col)) {
			spawn_projectile(state, plant->coord.row, (float)plant->coord.col + 0.65f);
			plant->action_timer += state->config->peashooter_fire_interval;
		}
	}
}

static void step_projectiles(GameState *state, float dt) {
	for (int i = 0; i < PVZ_MAX_PROJECTILES; ++i) {
		if (!state->projectiles[i].active) {
			continue;
		}

		Projectile *projectile = &state->projectiles[i];
		const float previous_x = projectile->x;
		projectile->x += projectile->speed * dt;

		// Find closest zombie for projectile to hit
		//  (perf note: mostly fine, as we don't have too many zombies)
		Zombie *target = NULL;
		float best_x = 9999.0f;
		for (int j = 0; j < PVZ_MAX_ZOMBIES; ++j) {
			Zombie *zombie = &state->zombies[j];
			if (!zombie->active || zombie->lane != projectile->lane) {
				continue;
			}
			if (zombie->x + 0.35f < previous_x || zombie->x - 0.35f > projectile->x) {
				continue;
			}
			if (zombie->x < best_x) {
				best_x = zombie->x;
				target = zombie;
			}
		}

		if (target) {
			damage_zombie(target, projectile->damage);
			projectile->active = false;
			continue;
		}

		if (projectile->x > (float)state->config->cols + 0.75f) {
			projectile->active = false;
		}
	}
}

static void step_zombies(GameState *state, float dt) {
	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (!state->zombies[i].active) {
			continue;
		}

		Zombie *zombie = &state->zombies[i];
		const int target_col = (int)floorf(zombie->x);
		bool attacking = false;

		if (target_col >= 0 && target_col < state->config->cols && zombie->lane >= 0 &&
			zombie->lane < state->config->rows) {

			// Check if can attack plant
			const int plant_index = state->plant_grid[zombie->lane][target_col];
			if (plant_index >= 0 && state->plants[plant_index].active && zombie->x <= (float)target_col + 0.82f) {
				attacking = true;
				zombie->attack_timer -= dt;
				if (zombie->attack_timer <= 0.0f) {
					state->plants[plant_index].health -= state->config->zombie_bite_damage;
					zombie->attack_timer += state->config->zombie_attack_interval;
					if (state->plants[plant_index].health <= 0) {
						remove_plant(state, plant_index);
					}
				}
			}
		}

		if (!attacking) {
			zombie->x -= zombie->speed * dt;
			zombie->attack_timer = state->config->zombie_attack_interval;
		}

		if (zombie->x < -0.1f) {
			state->status = GAME_STATUS_LOST;
		}
	}
}

bool game_coord_in_bounds(const GameState *state, BoardCoord coord) {
	return coord.row >= 0 && coord.row < state->config->rows && coord.col >= 0 && coord.col < state->config->cols;
}

void game_init(GameState *state, const GameConfig *config) {
	memset(state, 0, sizeof(*state));
	state->config = config;
	game_reset(state);
}

void game_reset(GameState *state) {
	clear_state_arrays(state);
	state->sun_count = state->config->starting_sun;
	state->selected_plant = PLANT_PEASHOOTER;
	state->status = GAME_STATUS_RUNNING;
	state->paused = false;
	state->last_command_result = GAME_COMMAND_RESULT_OK;
	state->spawn_timer = 1.3f;
	state->wave_cursor = 0;

	if (state->config->start_with_demo_layout) {
		spawn_demo_plants(state);
	}
}

GameCommandResult game_apply_command(GameState *state, GameCommand command) {
	GameCommandResult result = GAME_COMMAND_RESULT_IGNORED;

	switch (command.type) {
	case GAME_COMMAND_SELECT_PLANT:
		if (command.plant_type != PLANT_NONE) {
			state->selected_plant = command.plant_type;
			result = GAME_COMMAND_RESULT_OK;
		}
		break;
	case GAME_COMMAND_PLACE_PLANT: {
		const PlantType plant_type = command.plant_type != PLANT_NONE ? command.plant_type : state->selected_plant;
		if (plant_type == PLANT_NONE) {
			result = GAME_COMMAND_RESULT_NO_SELECTION;
			break;
		}
		if (!game_coord_in_bounds(state, command.coord)) {
			result = GAME_COMMAND_RESULT_OUT_OF_BOUNDS;
			break;
		}
		if (state->plant_grid[command.coord.row][command.coord.col] >= 0) {
			result = GAME_COMMAND_RESULT_OCCUPIED;
			break;
		}
		const int cost = game_get_plant_cost(state, plant_type);
		if (state->sun_count < cost) {
			result = GAME_COMMAND_RESULT_NOT_ENOUGH_SUN;
			break;
		}
		if (place_plant_internal(state, plant_type, command.coord, true)) {
			result = GAME_COMMAND_RESULT_OK;
		}
		break;
	}
	case GAME_COMMAND_REMOVE_PLANT:
		if (!game_coord_in_bounds(state, command.coord)) {
			result = GAME_COMMAND_RESULT_OUT_OF_BOUNDS;
			break;
		}
		if (state->plant_grid[command.coord.row][command.coord.col] < 0) {
			result = GAME_COMMAND_RESULT_NOT_FOUND;
			break;
		}
		remove_plant(state, state->plant_grid[command.coord.row][command.coord.col]);
		result = GAME_COMMAND_RESULT_OK;
		break;
	case GAME_COMMAND_TOGGLE_PAUSE:
		if (state->status == GAME_STATUS_RUNNING) {
			state->paused = !state->paused;
			result = GAME_COMMAND_RESULT_OK;
		}
		break;
	case GAME_COMMAND_RESTART:
		game_reset(state);
		result = GAME_COMMAND_RESULT_OK;
		break;
	case GAME_COMMAND_NONE:
	default:
		result = GAME_COMMAND_RESULT_IGNORED;
		break;
	}

	state->last_command_result = result;
	return result;
}

bool game_has_active_zombies(const GameState *state) {
	for (int i = 0; i < PVZ_MAX_ZOMBIES; ++i) {
		if (state->zombies[i].active) {
			return true;
		}
	}
	return false;
}

void game_step(GameState *state, float dt) {
	if (state->paused || state->status != GAME_STATUS_RUNNING) {
		return;
	}

	state->spawn_timer -= dt;
	if (state->spawn_timer <= 0.0f && state->wave_cursor < zombie_total_count()) {
		spawn_next_zombie(state);
		state->spawn_timer += state->config->zombie_spawn_interval;
	}

	step_suns(state, dt);
	step_plants(state, dt);
	step_projectiles(state, dt);
	step_zombies(state, dt);

	if (state->status == GAME_STATUS_RUNNING && state->wave_cursor >= zombie_total_count() &&
		!game_has_active_zombies(state)) {
		state->status = GAME_STATUS_WON;
	}
}

const char *pvz_get_plant_name(PlantType plant_type) {
	switch (plant_type) {
	case PLANT_SUNFLOWER:
		return "Sunflower";
	case PLANT_PEASHOOTER:
		return "Peashooter";
	case PLANT_WALLNUT:
		return "Wallnut";
	case PLANT_NONE:
	default:
		return "None";
	}
}

const char *pvz_get_zombie_name(ZombieType zombie_type) {
	switch (zombie_type) {
	case ZOMBIE_REGULAR:
		return "Regular";
	case ZOMBIE_CONE:
		return "Cone";
	case ZOMBIE_BUCKETHEAD:
		return "Bucket";
	case ZOMBIE_NONE:
	default:
		return "None";
	}
}
