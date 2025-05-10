cmake_minimum_required(VERSION 3.13.2)

set(PROJ_NAME positron)
project(${PROJ_NAME} CXX C)
set(EXE_NAME ${PROJ_NAME})
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(
	src
	vendor/imgui
	vendor/imgui/backends/
	)

set(SDL_SHARED ON)
set(SDL_STATIC OFF)
add_subdirectory(vendor/SDL)

set(LUA_PATH vendor/lua-5.4.7)
add_library(lua STATIC
	${LUA_PATH}/lapi.c
	${LUA_PATH}/lauxlib.c
	${LUA_PATH}/lbaselib.c
	${LUA_PATH}/lcode.c
	${LUA_PATH}/lcorolib.c
	${LUA_PATH}/lctype.c
	${LUA_PATH}/ldblib.c
	${LUA_PATH}/ldebug.c
	${LUA_PATH}/ldo.c
	${LUA_PATH}/ldump.c
	${LUA_PATH}/lfunc.c
	${LUA_PATH}/lgc.c
	${LUA_PATH}/linit.c
	${LUA_PATH}/liolib.c
	${LUA_PATH}/llex.c
	${LUA_PATH}/lmathlib.c
	${LUA_PATH}/lmem.c
	${LUA_PATH}/loadlib.c
	${LUA_PATH}/lobject.c
	${LUA_PATH}/lopcodes.c
	${LUA_PATH}/loslib.c
	${LUA_PATH}/lparser.c
	${LUA_PATH}/lstate.c
	${LUA_PATH}/lstring.c
	${LUA_PATH}/lstrlib.c
	${LUA_PATH}/ltable.c
	${LUA_PATH}/ltablib.c
	${LUA_PATH}/ltm.c
	${LUA_PATH}/lundump.c
	${LUA_PATH}/lutf8lib.c
	${LUA_PATH}/lvm.c
	${LUA_PATH}/lzio.c
)
target_include_directories(lua PUBLIC ${LUA_PATH})

add_library(libfmsynth STATIC
	vendor/libfmsynth/src/fmsynth.c
)
target_include_directories(libfmsynth PUBLIC vendor/libfmsynth/include)

add_library(thirdparty STATIC
	src/thirdparty/sqlite3.c
	src/thirdparty/miniz.c
	src/thirdparty/gl.c
	src/thirdparty/pathfind.cpp
)

target_include_directories(thirdparty PUBLIC
	src/thirdparty
)

add_executable(${PROJ_NAME}
	vendor/imgui/imgui.cpp
	vendor/imgui/imgui_demo.cpp
	vendor/imgui/imgui_draw.cpp
	vendor/imgui/imgui_tables.cpp
	vendor/imgui/imgui_widgets.cpp
	vendor/imgui/backends/imgui_impl_sdl3.cpp
	vendor/imgui/backends/imgui_impl_opengl3.cpp
	vendor/imgui/misc/cpp/imgui_stdlib.cpp
	src/thirdparty/ImGuiFileDialog.cpp
	src/main.cpp
	src/posi.cpp
	src/gpu.cpp
	src/input.cpp
	src/db.cpp
	src/script.cpp
	src/apu.cpp
	src/chip.cpp
	src/render.cpp
)

target_link_libraries(${PROJ_NAME} SDL3::SDL3 lua libfmsynth thirdparty)

set_target_properties(${PROJ_NAME} PROPERTIES
	LINKER_LANGUAGE CXX
)

target_compile_options(${EXE_NAME} PUBLIC 
	-g -Wall 
	$<$<CONFIG:Debug>:-g> 
	$<$<CONFIG:Release>: -O2>
)
target_link_options(${EXE_NAME} PUBLIC 
	$<$<CONFIG:RELEASE>: -s>
)
