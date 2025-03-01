#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>

#include <SDL3/SDL.h>
#include "argparse.hpp"

#include "posi.h"

/*
	The goal is to be able to write RPG games, racing games, platformers, strategy games.
*/

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Event event;
constexpr auto msPerTick = 1000 / 60;
uint64_t targetTime;

constexpr SDL_AudioSpec spec = {.format = SDL_AUDIO_S16LE, .channels=2, .freq=44100};
SDL_AudioDeviceID audioID;
SDL_AudioStream* stream;

void destroyPosiSDL() {
	posi_poweroff();
	
	SDL_DestroyAudioStream(stream);
	SDL_CloseAudioDevice(audioID);
	
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);	
	SDL_Quit();
}

int main(int argc, char *argv[])
{
	argparse::ArgumentParser program("Positron");
	program.add_argument("file")
		.nargs(argparse::nargs_pattern::optional)
		.default_value(std::string("World"));
	try {
		program.parse_args(argc, argv);
	  }
	  catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		return 1;
	  }
	
	posi_poweron();
	
	int16_t* audioBuffer;
	audioBuffer = posi_audiofeed();
	auto fileName = program.get<std::string>("file");
	
	if (!posi_load(fileName)){
		destroyPosiSDL();
		return 1;
	}
	
	bool done = false;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	window = SDL_CreateWindow("Positron", screenWidth*3, screenHeight*3, 
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	renderer = SDL_CreateRenderer(window, nullptr);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	
	audioID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
	stream = SDL_CreateAudioStream(&spec, nullptr);
	SDL_BindAudioStream(audioID, stream);
	
	targetTime = SDL_GetTicks() + msPerTick;
	while(!done) {
		while(SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				done = true;
			} else if (event.type == SDL_EVENT_KEY_DOWN) {
				// TODO
			} else if (event.type == SDL_EVENT_KEY_UP) {
				// TODO
			}
		}
		 
		 if(!posi_run()) {
			 done = true;
		 }

		SDL_PutAudioStreamData(stream, audioBuffer, audioFramesPerTick *2 * 2);
		
		void* texturePixels;
		int pitch;
		SDL_LockTexture(texture, nullptr, &texturePixels, &pitch);
		posi_redraw((uint32_t*)texturePixels);
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
	
	destroyPosiSDL();
	
	return 0;
}