#pragma once
#include <string>
#include <vector>

struct Argument {
	std::string OutputName = "";
	std::vector<std::string> Names = {};

	std::string Value = "";
};

class ArgumentParser {
public:
	std::vector<Argument> T;
}


/*
	auto Parser = ArgumentParser();

	Parser.Config({"-", "--"});
	Parser.Add("HelpFlag", {"h", "help"},    ArgumentParser::Types::FLAG | ArgumentParser::Flags::HALT);
	Parser.Add("Threads",  {"t", "threads"}, ArgumentParser::Types::INT16, std::thread::get_hardware_concurrency());
	Parser.Add("Ants",     {"a", "ant"},     ArgumentParser::Types::STRING | ArgumentParser::Flags::INCLUSIVE);

	auto Status = Parser.Parse(argv, argc);
	// ... check status

	if (Parser.Get<bool>("HelpFlag")) {
		std::cout << c_HelpMessage << '\n';
		return 0;
	}

	uint16_t Threads = Parser.Get<uint16_t>("Threads");
	auto Ants = Parser.Get<std::vector<std::string>>("Ants");

*/
/*
#include "Types/Vector.h"
#include "Types/Ant.h"

#include "Common.h"

struct Configuration {
	std::vector<Ant<CELL_TYPE, SIZE_TYPE>> Ants;
	std::vector<DirectionEnum> StateMachine;
	Vector2<SIZE_TYPE> GridSize;
};



bool ParseArguments(int ArgCount, const char* Args[], Configuration* Configs) {
	const char* c_HelpString = 
	"LangtonsAnt is a utility to evaluate the Langton's ant cellular automata.\n"
	"Usage:"
	"\n\t-h/--help Prints this message"
	"\n\t-h/--help Prints this message"
	;

	for (int i = 0; i < ArgCount; i++) {
		const char
			*Current = Args[i],
			*Next = (i + 1 < ArgCount) ? Args[i + 1] : nullptr;
		
		if (strlen(Current) < 2) continue; // Invalid argument

		bool SingleArgument = Current[0] == '-' && Current[1] != '-'; // "-X" Argument type
		bool DoubleArgument = Current[0] == '-' && Current[1] == '-'; // "--X" Argument type

		if (SingleArgument) {
			Current += 1;

			switch (*Current) {
				case 'x': Configs->GridSize.X = std::stoi(Next); break;
				case 'y': Configs->GridSize.Y = std::stoi(Next); break;
				case 't': Configs.Threads = std::stoi(Next); break;
				//case 'p'
			}
		}
		else if (DoubleArgument) {
			Current += 2;
			// lookup table time ig?
		}
	}
}
*/