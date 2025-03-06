cmake_minimum_required(VERSION 3.13.2)

set(PROJ_NAME positron)
project(${PROJ_NAME} CXX C)
set(EXE_NAME ${PROJ_NAME})
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(src)

set(SDL_SHARED ON)
set(SDL_STATIC OFF)
add_subdirectory(vendor/SDL)

add_library(lua STATIC
	vendor/lua/lapi.c
	vendor/lua/lauxlib.c
	vendor/lua/lbaselib.c
	vendor/lua/lcode.c
	vendor/lua/lcorolib.c
	vendor/lua/lctype.c
	vendor/lua/ldblib.c
	vendor/lua/ldebug.c
	vendor/lua/ldo.c
	vendor/lua/ldump.c
	vendor/lua/lfunc.c
	vendor/lua/lgc.c
	vendor/lua/linit.c
	vendor/lua/liolib.c
	vendor/lua/llex.c
	vendor/lua/lmathlib.c
	vendor/lua/lmem.c
	vendor/lua/loadlib.c
	vendor/lua/lobject.c
	vendor/lua/lopcodes.c
	vendor/lua/loslib.c
	vendor/lua/lparser.c
	vendor/lua/lstate.c
	vendor/lua/lstring.c
	vendor/lua/lstrlib.c
	vendor/lua/ltable.c
	vendor/lua/ltablib.c
	vendor/lua/ltests.c
	vendor/lua/ltm.c
	vendor/lua/lundump.c
	vendor/lua/lutf8lib.c
	vendor/lua/lvm.c
	vendor/lua/lzio.c
)
target_include_directories(lua PUBLIC vendor/lua)

add_executable(${PROJ_NAME}
	src/thirdparty/sqlite3.c
	src/thirdparty/m4p.c
	src/thirdparty/miniz.c
	src/main.cpp
	src/posi.cpp
	src/gpu.cpp
	src/input.cpp
	src/db.cpp
	src/script.cpp
)

target_link_libraries(${PROJ_NAME} SDL3::SDL3 lua)

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
