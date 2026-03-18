#pragma once

#include "app.h"
#include "input.h"

void raylib_poll_input(const AppContext *app, InputFrame *input);
void raylib_render_view(AppContext *app, const RenderView *view);
void raylib_frontend_shutdown(void);
