#include "global.h" 

static void display_scale(void){
    int ww, wh, dw, dh;
	SDL_GetWindowSize(_sdl.window, &ww, &wh);
	SDL_GetRendererOutputSize(_sdl.renderer, &dw, &dh);   // ← this is the real pixel buffer size
	float scale_x = (float)dw / ww;
	float scale_y = (float)dh / wh;
	char info[128];
	snprintf(
		info,
		sizeof(info),
		"Logical: %dx%d   Physical: %dx%d   Scale: %.1fx%.1f",
		ww, wh,
		dw, dh,
		scale_x, scale_y
	);
	add_terminal_line(info, LINE_FLAG_SYSTEM);
	
	/* Fix to rescale */
	//SDL_RenderSetScale(_sdl.renderer, 1.0f / scale_x, 1.0f / scale_y);
	//app.terminal.settings.font = TTF_OpenFont(FONT_PATH, (int)(app.config.font_size * scale_x + 0.5f));
}
