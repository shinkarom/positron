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
	posiPoweroff();
	
	SDL_DestroyAudioStream(stream);
	SDL_CloseAudioDevice(audioID);
	
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);	
	SDL_Quit();
}

SDL_FRect calcRect(SDL_FRect srcRect) {
	int windowWidth, windowHeight;
    SDL_GetCurrentRenderOutputSize(renderer, &windowWidth, &windowHeight);
	float scaleX = (float)windowWidth / screenWidth;
    float scaleY = (float)windowHeight / screenHeight;
	float scale = scaleX < scaleY ? scaleX : scaleY;
    float destWidth = screenWidth * scale;
    float destHeight = screenHeight * scale;
    float destX = (windowWidth - destWidth) / 2; // Center horizontally
    float destY = (windowHeight - destHeight) / 2; // Center vertically
	SDL_FRect destRect;
    destRect.x = destX;
    destRect.y = destY;
    destRect.w = destWidth;
    destRect.h = destHeight;
	return destRect;
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
	
	posiPoweron();
	
	int16_t* audioBuffer;
	audioBuffer = posiAudiofeed();
	auto fileName = program.get<std::string>("file");
	
	if (!posiLoad(fileName)){
		destroyPosiSDL();
		return 1;
	}
	
	bool done = false;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	window = SDL_CreateWindow("Positron", screenWidth*3, screenHeight*3, 
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowMinimumSize(window, maxScreenWidth, maxScreenHeight);
	renderer = SDL_CreateRenderer(window, nullptr);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, maxScreenWidth, maxScreenHeight);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	
	SDL_FRect texRect, winRect;
	
	
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
			} else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
				texRect.x = 0;
				texRect.y = 0;
				texRect.w = screenWidth;
				texRect.h = screenHeight;
				winRect = calcRect(texRect);
			}
		}
		 
		 if(!posiRun()) {
			 done = true;
		 }

		SDL_PutAudioStreamData(stream, audioBuffer, audioFramesPerTick *2 * 2);
		
		
		
		void* texturePixels;
		int pitch;
		SDL_LockTexture(texture, nullptr, &texturePixels, &pitch);
		posiRedraw((uint32_t*)texturePixels);
		SDL_UnlockTexture(texture);
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, texture, &texRect, &winRect);
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