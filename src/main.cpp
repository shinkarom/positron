#include <iostream>
#include <cstdint>
#include <cstring>
#include <string>

#include "thirdparty/argparse.hpp"

#include "posi.h"
#include "render.h"

/*
	The goal is to be able to write RPG games, racing games, platformers, strategy games.
*/

int main(int argc, char *argv[])
{
	argparse::ArgumentParser program("Positron");
	program.add_argument("file");
		//.nargs(argparse::nargs_pattern::optional)
		//.default_value(std::string(""));
	try {
		program.parse_args(argc, argv);
	  }
	  catch (const std::exception& err) {
		std::cerr << err.what() << std::endl;
		std::cerr << program;
		return 1;
	  }
	auto fileName = program.get<std::string>("file");
	
	posiPoweron();
	
	bool done = false;
	if (!posiSDLLoadFile(fileName)){
		posiSDLDestroy();
		return 1;
	}
	
	posiSDLInit();

	while(!done) {	
		done = posiSDLTick();
		posiSDLCountTime();
	}
	
	posiSDLDestroy();
	
	return 0;
}