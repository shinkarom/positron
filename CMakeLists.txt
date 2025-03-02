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

add_library(qjs STATIC
	vendor/quickjs/libbf.c
	vendor/quickjs/libregexp.c
	vendor/quickjs/libunicode.c
	vendor/quickjs/quickjs.c
	vendor/quickjs/cutils.c
)

target_include_directories(qjs PUBLIC vendor/quickjs)

add_executable(${PROJ_NAME}
	src/sqlite3.c
	src/m4p.c
	src/miniz.c
	src/main.cpp
	src/posi.cpp
	src/gpu.cpp
)

target_link_libraries(${PROJ_NAME} SDL3::SDL3 qjs)

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
