#pragma once
#include <cstring>
#include <string>
#include <vector>

#include "Types/Vector.h"
#include "Types/Ant.h"

#include "Common.h"

struct Configuration {
	std::vector<Ant<CELL_TYPE, SIZE_TYPE>> Ants;
	std::vector<DirectionEnum> StateMachine;
	Vector2<SIZE_TYPE> GridSize;
};


void ParseArguments(int ArgCount, const char* Args[], Configuration* Configs) {
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
				//case 't': Configs.Threads = std::stoi(Next); break;
				//case 'p'
			}
		}
		else if (DoubleArgument) {
			Current += 2;
			// lookup table time ig?
		}
	}
}