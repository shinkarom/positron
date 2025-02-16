#include <iostream>
#include <cstdint>
#include <cstring>

#include "sqlite3.h"
#include <SDL3/SDL.h>
#include <quickjs.h>

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Event event;
constexpr auto msPerTick = 1000 / 60;
uint64_t targetTime;

constexpr SDL_AudioSpec spec = {.format = SDL_AUDIO_S16LE, .channels=2, .freq=44100};
SDL_AudioDeviceID audioID;
SDL_AudioStream* stream;
constexpr auto audioFramesPerTick = 44100 / 60;
int16_t audioBuffer[audioFramesPerTick*2];

sqlite3* db;

JSRuntime* runtime;
JSContext* context;

constexpr auto screenWidth = 320;
constexpr auto screenHeight = 240;
uint32_t frameBuffer[screenWidth * screenHeight];

int main(int argc, char *argv[])
{
	for(int i = 0; i< audioFramesPerTick*2; i++) {
		audioBuffer[i] = 0;
	}
	
	bool done = false;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
	window = SDL_CreateWindow("Positron", screenWidth*3, screenHeight*3, 
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	renderer = SDL_CreateRenderer(window, nullptr);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	
	audioID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
	stream = SDL_CreateAudioStream(&spec, nullptr);
	SDL_BindAudioStream(audioID, stream);
	
	sqlite3_open(":memory:", &db);
	
	runtime = JS_NewRuntime();
	context = JS_NewContext(runtime);
	
	targetTime = SDL_GetTicks() + msPerTick;
	while(!done) {
		while(SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				done = true;
			}
		}
		
		SDL_PutAudioStreamData(stream, &audioBuffer, audioFramesPerTick *2 * 2);
		
		void* texturePixels;
		int pitch;
		frameBuffer[50*screenWidth+50] = 0xFFFFFFFF;
		SDL_LockTexture(texture, nullptr, &texturePixels, &pitch);
		memcpy(texturePixels, frameBuffer,screenHeight*pitch);
		SDL_UnlockTexture(texture);
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(renderer);
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
	
	SDL_DestroyAudioStream(stream);
	SDL_CloseAudioDevice(audioID);
	
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);	
	SDL_Quit();
	return 0;
}