#include "render.h"

#include <iostream>

#include <SDL3/SDL.h>
#include "gl.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"

#include "posi.h"

SDL_Window* window;
SDL_GLContext gl_context;
GLuint gameTexture;
ImGuiIO io;
uint32_t* videoBuffer;
constexpr auto msPerTick = 1000 / 60;
uint64_t targetTime;

constexpr SDL_AudioSpec spec = {.format = SDL_AUDIO_S16LE, .channels=2, .freq=audioSampleRate};
SDL_AudioDeviceID audioID;
SDL_AudioStream* stream;
int16_t* audioBuffer;

constexpr float statusBarHeight = 20.0f;
bool lastFullscreenState = false;

const uint64_t perf_frequency = SDL_GetPerformanceFrequency();
uint64_t last_time = SDL_GetPerformanceCounter();

double accumulator = 0.0;
const double fixed_delta_time = 1.0 / 60.0; // Our target update rate (in seconds)

bool isFileLoaded;
bool isFullscreen;
bool isPaused;
bool wasPaused;

std::string filename;

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

bool posiSDLLoadFile(std::string fileName) {
	auto result = posiLoad(fileName);
	isFileLoaded = result;
	filename = result ? fileName : "";
	return result;
}

void posiSDLUnload() {
	posiUnload();
	isFileLoaded = false;
	filename = "";
}

void initOpenGL() {
	gl_context = SDL_GL_CreateContext(window);
	gladLoadGL(SDL_GL_GetProcAddress);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(0); 

	glGenTextures(1, &gameTexture);
	glBindTexture(GL_TEXTURE_2D, gameTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight,0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
}

void destroyOpenGL() {
	glDeleteTextures(1, &gameTexture);
	SDL_GL_DestroyContext(gl_context);
}

void initImGUI() {
	ImGui::CreateContext();
	io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.IniFilename = nullptr;
	
	ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init("#version 330");
}

void posiSDLInit() {
	audioBuffer = posiAudiofeed();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	
	window = SDL_CreateWindow("Positron", screenWidth*3, screenHeight*3, 
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_SetWindowMinimumSize(window, screenWidth, screenHeight);
	
	audioID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
	stream = SDL_CreateAudioStream(&spec, nullptr);
	SDL_BindAudioStream(audioID, stream);
	
	isFullscreen = false;
	isPaused = false;
	videoBuffer = gpuGetBuffer();
	initOpenGL();
	initImGUI();
}

bool drawMenuBar() {
	bool result = false;
	if (!isFullscreen &&ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open")) {
				wasPaused = isPaused;
				isPaused = true;
				IGFD::FileDialogConfig config;
				config.path = "";
				ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Open File", ".posi", config);
				// Open file action
			}
			if (ImGui::MenuItem("Unload")) {
				posiSDLUnload();
			}
			if (ImGui::MenuItem("Exit")) {
				result = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View")) {
			if (ImGui::MenuItem("Fullscreen")) {
				isFullscreen = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Emulation")) {
			if (ImGui::MenuItem("Pause", nullptr, isFileLoaded && isPaused)) {
				isPaused = !isPaused;
			}
			if (ImGui::MenuItem("Reset", nullptr, false)) {
				posiReset();
				
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}
	
	// display
	  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
		  bool success = false;
		if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
		  std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
		  std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
		  success = posiSDLLoadFile(filePathName);
		}
		
		// close
		ImGuiFileDialog::Instance()->Close();
		if(!success) {
			isPaused = wasPaused;
		} else {
			isPaused = false;
		}
	  }
	
	return result;
}

bool posiSDLRender() {
	bool result = false;
	if (isFullscreen != lastFullscreenState) {
		SDL_SetWindowFullscreen(window, isFullscreen);
		lastFullscreenState = isFullscreen;
	}
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_BGRA, GL_UNSIGNED_BYTE, videoBuffer);
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(w, h));
	auto flags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoBackground;
	if(!isFullscreen) {
		flags |= ImGuiWindowFlags_MenuBar;
	}
	ImGui::Begin("Positron", nullptr, flags);
	
	if(drawMenuBar()) {
		result = true;
	}
	
	ImVec2 availableSize = ImGui::GetContentRegionAvail();
		
	auto imageHeight = isFullscreen ? availableSize.y : availableSize.y - statusBarHeight;
	ImGui::BeginChild("ImageRegion", ImVec2(availableSize.x, imageHeight), false);
	ImVec2 imageSize = ImGui::GetContentRegionAvail();

	float scale = std::min(imageSize.x * 1.0f / screenWidth, imageSize.y * 1.0f / screenHeight);
	ImVec2 scaledSize = ImVec2(screenWidth * 1.0f * scale, screenHeight * 1.0f * scale);
	ImVec2 padding = ImVec2((imageSize.x - scaledSize.x) / 2.0f, (imageSize.y - scaledSize.y) / 2.0f);
	ImVec2 finalPos = ImVec2(ImGui::GetCursorPos().x+padding.x, ImGui::GetCursorPos().y+padding.y);
	ImGui::SetCursorPos(finalPos);

	// Render the image at the scaled size
	ImGui::Image(gameTexture, scaledSize);  
	ImGui::EndChild();
	
	if(!isFullscreen) {
		auto status = isFileLoaded ? (isPaused ? "Paused" : "Running") : "No file loaded";
		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - statusBarHeight);
		ImGui::BeginChild("StatusBar", ImVec2(availableSize.x, statusBarHeight), false);
		ImGui::Text("FPS: %.1f | %s", ImGui::GetIO().Framerate, status);  // Example status bar text
		ImGui::EndChild();
	}
	
	ImGui::End();
	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(window);
	
	return result;
}

bool posiSDLHandleEvents() {
	bool result = false;
	
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		ImGui_ImplSDL3_ProcessEvent(&event);
		if (event.type == SDL_EVENT_QUIT) {
			result = true;
		} else if (event.type == SDL_EVENT_KEY_DOWN) {
			
		} else if (event.type == SDL_EVENT_KEY_UP) {
			if(event.key.scancode == SDL_SCANCODE_F10) {
				if(!posiReset()){
					result = true;
				}
			}
			if(event.key.scancode == SDL_SCANCODE_F11){
				isPaused = !isPaused;
				if(isPaused) apuClearBuffer();
			}
			if((event.key.scancode == SDL_SCANCODE_F12)||(isFullscreen&&event.key.scancode == SDL_SCANCODE_ESCAPE)) {
				isFullscreen = !isFullscreen;
			}
		}
	}
	
	return result;
}

bool posiSDLEmulate() {
	bool result = false;
	
	if(!isPaused && !io.WantCaptureKeyboard) {
		auto kbState = SDL_GetKeyboardState(nullptr);
		for (int i = 0; i < numInputButtons; i++) {
			posiUpdateButton(i, kbState[inputScancodes[i]]);
		 }
	}
	
	 if(!isPaused) {
		if(!posiRun()) {
			result = true;
		} 
	 }
	 
	 
	SDL_PutAudioStreamData(stream, audioBuffer, audioFramesPerTick *2 * 2); 
	return result;
}

bool posiSDLTick() {	
	if(posiSDLHandleEvents()) {
		return true;
	}
	
    uint64_t current_time = SDL_GetPerformanceCounter();
    double delta_time = (double)(current_time - last_time) / perf_frequency;
    last_time = current_time;

    accumulator += delta_time;
	if (accumulator > 0.25) {
        accumulator = 0.25;
    }
	

    while (accumulator >= fixed_delta_time) {
        if(posiSDLEmulate()) {
			return true;
		}

        accumulator -= fixed_delta_time;
    }

	posiSDLRender();
	
	return false;
}

void posiSDLDestroy() {
	posiPoweroff();
	
	SDL_DestroyAudioStream(stream);
	SDL_CloseAudioDevice(audioID);
	
	destroyOpenGL();
	
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	
	SDL_DestroyWindow(window);	
	SDL_Quit();
}