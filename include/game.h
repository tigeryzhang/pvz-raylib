#pragma once

#include <stdbool.h>

#include "game_types.h"
#include "pvz_config.h"

typedef struct {
	bool active;
	PlantType type;
	BoardCoord coord;
	int health;
	float action_timer;
} Plant;

typedef struct {
	bool active;
	ZombieType type;
	int lane;
	float x;
	int health;
	int armor;
	float speed;
	float attack_timer;
} Zombie;

typedef struct {
	bool active;
	int lane;
	float x;
	int damage;
	float speed;
} Projectile;

typedef struct {
	bool active;
	float x;
	float y;
	int value;
	float collect_timer;
} Sun;

// TODO: Add Lawnmowers
typedef struct {
	const GameConfig *config;
	int sun_count;
	PlantType selected_plant;
	float seed_cooldowns[PVZ_NUM_PLANT_TYPES];
	GameStatus status;
	bool paused;
	GameCommandResult last_command_result;
	float spawn_timer;
	int wave_cursor;
	int plant_grid[PVZ_MAX_ROWS][PVZ_MAX_COLS];
	Plant plants[PVZ_MAX_PLANTS];
	Zombie zombies[PVZ_MAX_ZOMBIES];
	Projectile projectiles[PVZ_MAX_PROJECTILES];
	Sun suns[PVZ_MAX_SUNS];
} GameState;

void game_init(GameState *state, const GameConfig *config);
void game_reset(GameState *state);
GameCommandResult game_apply_command(GameState *state, GameCommand command);
void game_step(GameState *state, float delta);
bool game_coord_in_bounds(const GameState *state, BoardCoord coord);
bool game_has_active_zombies(const GameState *state);
