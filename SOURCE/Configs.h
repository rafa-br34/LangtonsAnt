#pragma once
#include <string>
#include <vector>
#include <array>
#include <regex>

#include <argparse/argparse.hpp>

#include "Types/Vector.h"
#include "Types/Ant.h"

// @todo A ParserState class could be a good idea
namespace Configs {
	union EvaluationTime

	enum class ParserStatus : uint8_t {
		OK = 0,
		NO_MATCHES,
		INVALID_FORMAT,
		INVALID_FLAG
	};

	std::vector<DirectionEnum> ParseStateMachine(std::string String) {
		std::vector<DirectionEnum> Result;

		size_t Skip = 0;
		int8_t ParseDirection = 0;

		for (auto Character : String) {
			if (ParseDirection != 0) { // Parse direction
				const uint8_t c_SkipSize[] = { 1, 1, 2 }; // How many characters to skip
				const char c_Angles[] = { '4', '9', '1' }; // 45, 90, 135

				for (uint8_t i = 0; i < sizeof(c_Angles) / sizeof(char); i++) {
					if (Character == c_Angles[i]) {
						Result.push_back(DirectionEnum((i + 1) * ParseDirection));
						Skip += c_SkipSize[i];
						break;
					}
				}

				// If skip is 0 then we didn't find anything, so presume R90/L90
				if (Skip == 0)
					Result.push_back(DirectionEnum(2 * ParseDirection));

				ParseDirection = 0; // Stop parsing direction
			}
			if (Skip > 0) { Skip--; continue; }
			
			// Parse command
			switch (Character) {
				case 'R':
					ParseDirection =  1; break; // Prepare to parse right direction
				case 'L':
					ParseDirection = -1; break; // Prepare to parse left direction
				case 'U':
					Result.push_back(DirectionEnum::U); break;
				case 'C':
					Result.push_back(DirectionEnum::C); break;
				
				default:
					break;
			}
		}

		if (ParseDirection != 0)
			Result.push_back(DirectionEnum(2 * ParseDirection));
		
		return Result;
	}

	template<typename CellType, typename SizeType>
	ParserStatus ParseFlags(const std::string& String, Ant<CellType, SizeType>& AntObject) {
		for (auto Flag : String) {
			switch (Flag) {
				case 'W':
					AntObject.Wrap = true; break;
				
				default:
					return ParserStatus::INVALID_FLAG;
			}
		}

		return ParserStatus::OK;
	}

	template<typename CellType, typename SizeType>
	ParserStatus ParseAnt(const std::string& String, const std::vector<std::vector<DirectionEnum>>& StateMachines, Ant<CellType, SizeType>& AntObject) {
		std::smatch Matches;
		
		if (!std::regex_match(String, Matches, std::regex(R"((.)\(([\w\s,+-]*?)\))"))) return ParserStatus::NO_MATCHES;
		if (Matches.size() % 3 != 0) return ParserStatus::INVALID_FORMAT;

		for (size_t i = 0; i < Matches.size(); i += 3) {
			const auto& Value = Matches[i + 2];

			switch (Matches[i + 1].str().at(0)) {
				case 'P': { // Position
					std::sscanf(Value.str().c_str(), "%d,%d", &AntObject.Position.X, &AntObject.Position.Y);
					break;
				}
				case 'D': { // Direction
					std::sscanf(Value.str().c_str(), "%d,%d", &AntObject.Direction.X, &AntObject.Direction.Y);
					break;
				}
				case 'M': { // State machine
					if (std::regex_match(Value.str(), std::regex("\\d+"))) {
						int Index;
						if ((Index = std::stoi(Value.str())) + 1 >= StateMachines.size())
							return ParserStatus::INVALID_FORMAT;
						
						AntObject.StateMachine = StateMachines[Index];
					}
					else // Is the state machine a list?
						AntObject.StateMachine = ParseStateMachine(Value.str());
					
					break;
				}
				case 'S': { // Step size
					if (std::regex_match(Value.str(), std::regex("\\d+")))
						AntObject.StepSize = std::stoi(Value.str());
					else
						return ParserStatus::INVALID_FORMAT;
					
					break;
				}
				case 'F': { // Flags
					ParserStatus Result = ParseFlags(Value.str(), AntObject);
					
					if (Result != ParserStatus::OK)
						return Result;
					else
						break;
				}
				
				default:
					break;
			}
		}
		
		return ParserStatus::OK;
	}

	template<typename CellType=CELL_TYPE, typename SizeType=SIZE_TYPE>
	ParserStatus ParseAnts(std::string String, const std::vector<std::vector<DirectionEnum>>& StateMachines, std::vector<Ant<CellType, SizeType>>& Ants) {
		size_t Position = 0;

		while ((Position = String.find(';')) != std::string::npos) {
			Ant<CellType, SizeType> AntObject;

			ParserStatus Result = ParseAnt(String.substr(0, Position), StateMachines, AntObject);

			if (Result == ParserStatus::OK)
				Ants.push_back(Result);
			else
				return Result;
			
			String.erase(0, Position + 1);
		}
		
		return ParserStatus::OK;
	}

	ParserStatus ParseStateMachines(std::string String, std::vector<std::vector<DirectionEnum>>& StateMachines) {
		size_t Position = 0;

		while ((Position = String.find(';')) != std::string::npos) {
			StateMachines.push_back(ParseStateMachine(String.substr(0, Position)));
			String.erase(0, Position + 1);
		}

		StateMachines.push_back(ParseStateMachine(String.substr(0, String.size())));
		
		return ParserStatus::OK;
	}
}