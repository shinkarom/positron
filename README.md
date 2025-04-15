# Positron

A virtual machine for games.

## Specifications:

* screen resolution 256x256
* 64 tile pages of 256 8x8 sprites
* 32 tilemaps of 8x8 screens
* 8 channels of [libfmsynth](https://github.com/Themaister/libfmsynth) (FM synthesis)
* scripted in Lua
* SQLite file format

## Building

Positron is built using CMake. After downloading the repository:

* create a `build` folder
* from this folder execute `cmake .. -GNinja` (or any other builds system of your choice)
* from repository root execute `cmake --build build --config Debug` (use the path to the build folder. the configurations can be `Debug` or `Release`)
* the resulting executable will be in the `bin` folder of the repository
