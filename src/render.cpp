#include "render.h"

#include <SDL3/SDL.h>

#include "posi.h"

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
SDL_Event event;
constexpr auto msPerTick = 1000 / 60;
uint64_t targetTime;

constexpr SDL_AudioSpec spec = {.format = SDL_AUDIO_S16LE, .channels=2, .freq=audioSampleRate};
SDL_AudioDeviceID audioID;
SDL_AudioStream* stream;
int16_t* audioBuffer;

bool hasFullscreen;
bool isPaused;

constexpr SDL_Scancode inputScancodes[numInputButtons] = {
	SDL_SCANCODE_UP,
	SDL_SCANCODE_DOWN,
	SDL_SCANCODE_LEFT,
	SDL_SCANCODE_RIGHT,
	
	SDL_SCANCODE_RETURN,
	SDL_SCANCODE_RSHIFT,
	
	SDL_SCANCODE_Z,
	SDL_SCANCODE_X,
	SDL_SCANCODE_C,
	SDL_SCANCODE_A,
	SDL_SCANCODE_S,
	SDL_SCANCODE_D,
};

void posiSDLInit() {
	audioBuffer = posiAudiofeed();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	window = SDL_CreateWindow("Positron", screenWidth*3, screenHeight*3, 
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowMinimumSize(window, screenWidth, screenHeight);
	renderer = SDL_CreateRenderer(window, nullptr);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	
	audioID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
	stream = SDL_CreateAudioStream(&spec, nullptr);
	SDL_BindAudioStream(audioID, stream);
	
	hasFullscreen = false;
	isPaused = false;
}

bool posiSDLTick() {
	SDL_FRect texRect, winRect;
	
	bool done = false;
	while(SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			done = true;
		} else if (event.type == SDL_EVENT_KEY_DOWN) {
			
		} else if (event.type == SDL_EVENT_KEY_UP) {
			if(event.key.scancode == SDL_SCANCODE_F9) {
				posiAPISlotDeleteAll();
			}
			if(event.key.scancode == SDL_SCANCODE_F10) {
				if(!posiReset()){
					done = true;
				}
			}
			if(event.key.scancode == SDL_SCANCODE_F11){
				isPaused = !isPaused;
				if(isPaused) apuClearBuffer();
			}
			if((event.key.scancode == SDL_SCANCODE_F12)||(hasFullscreen&&event.key.scancode == SDL_SCANCODE_ESCAPE)) {
				hasFullscreen = !hasFullscreen;
				SDL_SetWindowFullscreen(window,hasFullscreen);
			}
		}
	}
	if(!isPaused) {
		auto kbState = SDL_GetKeyboardState(nullptr);
		for (int i = 0; i < numInputButtons; i++) {
			posiUpdateButton(i, kbState[inputScancodes[i]]);
		 }
	}
	 
	texRect.x = 0;
	texRect.y = 0;
	texRect.w = screenWidth;
	texRect.h = screenHeight;
	
	int windowWidth, windowHeight;
	SDL_GetCurrentRenderOutputSize(renderer, &windowWidth, &windowHeight);
	float scaleX = (float)windowWidth / texRect.w;
	float scaleY = (float)windowHeight / texRect.h;
	float scale = scaleX < scaleY ? scaleX : scaleY;
	float destWidth = texRect.w * scale;
	float destHeight = texRect.h * scale;
	float destX = (windowWidth - destWidth) / 2.0; // Center horizontally
	float destY = (windowHeight - destHeight) / 2.0; // Center vertically
	winRect.x = destX;
	winRect.y = destY;
	winRect.w = destWidth;
	winRect.h = destHeight;
	
	 if(isPaused) {
		 
	 } else
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
	return done;
}

void posiSDLCountTime() {
	auto currentTime = SDL_GetTicks();
	if(currentTime < targetTime) {
		SDL_Delay(targetTime - currentTime);
	}
	targetTime = SDL_GetTicks() + msPerTick;
}

void posiSDLDestroy() {
	posiPoweroff();
	
	SDL_DestroyAudioStream(stream);
	SDL_CloseAudioDevice(audioID);
	
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);	
	SDL_Quit();
}