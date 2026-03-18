#pragma once

#include "pvz_config.h"
#include "scene.h"

typedef struct {
	GameState game;
	float accumulator;
	RenderStatus status;
	float status_timer;
} PlaySceneState;

typedef struct {
	float elapsed;
} PlaceholderSceneState;

typedef struct AppContext {
	GameConfig config;
	DisplaySettings display_settings;
	bool quit_requested;
	SceneId active_scene_id;
	Scene *active_scene;
	Scene play_scene;
	Scene placeholder_scene;
	PlaySceneState play_state;
	PlaceholderSceneState placeholder_state;
	void (*render_view)(struct AppContext *app, const RenderView *view);
} AppContext;

void app_init(AppContext *app, const GameConfig *config);
void app_shutdown(AppContext *app);
void app_update(AppContext *app, const InputFrame *input, float frame_dt);
void app_build_view(AppContext *app, RenderView *view);
void app_render(AppContext *app, const RenderView *view);
void app_request_scene(AppContext *app, SceneId next_scene);
void play_scene_configure(Scene *scene, PlaySceneState *state);
void placeholder_scene_configure(Scene *scene, PlaceholderSceneState *state);
