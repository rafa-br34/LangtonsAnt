#pragma once
#include <string_view>
#include <optional>
#include <string>
#include <vector>
#include <chrono>
#include <array>
#include <regex>

#include <argparse/argparse.hpp>

#include "Types/Vector.h"
#include "Types/Ant.h"


namespace Configs {
	enum class TimingType : uint8_t {
		ITERATION_BASED,
		TIME_BASED
	};

	struct Timing {
		TimingType Type;
		union {
			size_t Iter;
			double Time;
		} Value;

		inline void SetTime(double Time) {
			Value.Time = Time;
			Type = TimingType::TIME_BASED;
		}

		inline void SetIter(size_t Iter) {
			Value.Iter = Iter;
			Type = TimingType::ITERATION_BASED;
		}
	};

	enum class ParserStatus : uint8_t {
		OK = 0,
		NO_MATCHES,
		INVALID_FORMAT,
		INVALID_VALUE,
		INVALID_FLAG
	};

	const char* ErrorCodes[] = {
		"None",
		"No matches",
		"Invalid format",
		"Invalid value",
		"Invalid flag"
	};

	std::optional<size_t> CharacterIndex(char Character, std::string_view Dictionary) {
		auto Iterator = std::find(Dictionary.begin(), Dictionary.end(), Character);

		if (Iterator != Dictionary.end()) {
			return std::distance(Dictionary.begin(), Iterator);
		}
		else {
			return std::nullopt;
		}
	}

	ParserStatus ParseTiming(const std::string& String, Timing* Condition) {
		std::smatch Matches;

		const double c_MultipliersTime[] = {
			1,    // s
			60,   // m
			3600, // h
			86400 // d
		};

		const size_t c_MultipliersIter[] = {
			1,         // i
			1000,      // k
			1000000,   // m
			1000000000 // b
		};

		if (!std::regex_match(String, Matches, std::regex("^(.)(\\d+)(.?)")))
			return ParserStatus::INVALID_FORMAT;
		
		auto Multiplier = Matches[3].str();
		auto Value      = (size_t)std::stoi(Matches[2].str());
		auto Type       = Matches[1].str();
		
		if (Type == "i" || Type == "I") {
			size_t MultiplierValue = 1;

			if (Multiplier.size() > 0) {
				auto Index = CharacterIndex(Multiplier.at(0), "ikmb");

				if (Index)
					MultiplierValue = c_MultipliersIter[*Index];
				else
					return ParserStatus::INVALID_FLAG;
			}

			Condition->SetIter(MultiplierValue * Value);
		}
		else if (Type == "t" || Type == "T") {
			double MultiplierValue = 1.0;

			if (Multiplier.size() > 0) {
				auto Index = CharacterIndex(Multiplier.at(0), "smhd");

				if (Index)
					MultiplierValue = c_MultipliersTime[*Index];
				else
					return ParserStatus::INVALID_FLAG;
			}

			Condition->SetTime(MultiplierValue * Value);
		}

		else
			return ParserStatus::INVALID_VALUE;

		return ParserStatus::OK;
	}

	std::vector<DirectionEnum> ParseStateMachine(const std::string& String) {
		std::vector<DirectionEnum> Result;

		int8_t Direction = 0;
		size_t Skip = 0;

		for (auto Character : String) {
			if (Direction != 0) {
				const uint8_t c_SkipSize[] = { 1, 1, 2 }; // How many characters to skip
				auto Index = CharacterIndex(Character, "491"); // 45, 90, 135

				if (Index) {
					Result.push_back(DirectionEnum((*Index + 1) * Direction));
					Skip += c_SkipSize[*Index];
				}

				// If skip is 0 then we didn't find anything to describe the angle, so presume R90/L90
				if (Skip == 0)
					Result.push_back(DirectionEnum(2 * Direction));

				Direction = 0; // Stop parsing direction
			}

			if (Skip > 0) { Skip--; continue; }
			
			// Parse command
			switch (Character) {
				case 'R':
					Direction =  1; break; // Prepare to parse right direction
				case 'L':
					Direction = -1; break; // Prepare to parse left direction
				case 'U':
					Result.push_back(DirectionEnum::U); break;
				case 'C':
					Result.push_back(DirectionEnum::C); break;
				
				default:
					break;
			}
		}

		if (Direction != 0)
			Result.push_back(DirectionEnum(2 * Direction));
		
		return Result;
	}

	template<typename Type>
	ParserStatus ParseVector2(const std::string& String, Vector2<Type>* Vector) {
		std::smatch Matches;

		if (!std::regex_match(String, Matches, std::regex(R"((-?\d+)\s*,\s*(-?\d+))")))
			return ParserStatus::INVALID_FORMAT;

		if (Matches.size() != 3)
			return ParserStatus::INVALID_FORMAT;
		
		Vector->X = (Type)std::stoi(Matches[1].str());
		Vector->Y = (Type)std::stoi(Matches[2].str());

		return ParserStatus::OK;
	}

	ParserStatus ParseFlags(const std::string& String, Ant& AntObject) {
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

	ParserStatus ParseAnt(const std::string& String, const std::vector<std::vector<DirectionEnum>>& StateMachines, Ant& AntObject) {
		std::smatch Matches;
		
		if (!std::regex_match(String, Matches, std::regex(R"((.)\(([\w\s,+-]*?)\))")))
			return ParserStatus::NO_MATCHES;
		
		if (Matches.size() % 3 != 0)
			return ParserStatus::INVALID_FORMAT;

		for (size_t i = 0; i < Matches.size(); i += 3) {
			const std::string& Value = Matches[i + 2].str();

			ParserStatus Result = ParserStatus::OK;

			switch (Matches[i + 1].str().at(0)) {
				case 'P': // Position
					Result = ParseVector2(Value, &AntObject.Position); break;

				case 'D': // Direction
					Result = ParseVector2(Value, &AntObject.Direction); break;
				
				case 'F': // Flags
					Result = ParseFlags(Value, AntObject); break;

				case 'M': { // State machine
					if (std::regex_match(Value, std::regex("\\d+"))) {
						auto Index = (size_t)std::stoi(Value);
						if (Index + 1 >= StateMachines.size())
							return ParserStatus::INVALID_FORMAT;
						
						AntObject.StateMachine = StateMachines[Index];
					}
					else // Is the state machine a list?
						AntObject.StateMachine = ParseStateMachine(Value);
					
					break;
				}

				case 'S': { // Step size
					if (std::regex_match(Value, std::regex("\\d+")))
						AntObject.StepSize = std::stoi(Value);
					else
						return ParserStatus::INVALID_FORMAT;
					
					break;
				}
				
				default:
					break;
			}

			if (Result != ParserStatus::OK)
				return Result;
		}
		
		return ParserStatus::OK;
	}

	ParserStatus ParseAnts(std::string String, const std::vector<std::vector<DirectionEnum>>& StateMachines, std::vector<Ant>& Ants) {
		size_t Position = 0;

		while ((Position = String.find(';')) != std::string::npos) {
			Ant AntObject;

			ParserStatus Result = ParseAnt(String.substr(0, Position), StateMachines, AntObject);

			if (Result == ParserStatus::OK)
				Ants.push_back(AntObject);
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