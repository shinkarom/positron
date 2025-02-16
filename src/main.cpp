#include <iostream>
#include <cstdint>

#include "sqlite3.h"
#include <SDL3/SDL.h>
#include <quickjs.h>

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Event event;
constexpr auto msPerTick = 1000 / 60;
uint64_t targetTime;

sqlite3* db;

JSRuntime* runtime;
JSContext* context;

int main(int argc, char *argv[])
{
	bool done = false;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
	window = SDL_CreateWindow("Positron", 500, 500, 
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	renderer = SDL_CreateRenderer(window, nullptr);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 500, 500);
	
	sqlite3_open(":memory:", &db);
	
	runtime = JS_NewRuntime();
	context = JS_NewContext(runtime);
	
	targetTime = SDL_GetTicks() + msPerTick;
	while(!done) {
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
		while(SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				done = true;
			}
		}
		auto currentTime = SDL_GetTicks();
		if(currentTime < targetTime) {
			SDL_Delay(targetTime - currentTime);
		}
		targetTime = SDL_GetTicks() + msPerTick;
	}
	// A small JavaScript program to evaluate
    const char *js_code = "3+2;";

    // Evaluate the JavaScript code
    JS_Eval(context, js_code, strlen(js_code), "<input>", JS_EVAL_TYPE_GLOBAL);
	
	JS_FreeContext(context);
	JS_FreeRuntime(runtime);
	
	sqlite3_close(db);
	
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);	
	SDL_Quit();
	return 0;
}