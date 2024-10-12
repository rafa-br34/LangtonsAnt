#include <algorithm>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <map>

#include <array>

#include <argparse/argparse.hpp>

#include "Encoding.h"
#include "Configs.h"
#include "Common.h"

#include "Types/SimulationState.h"
#include "Types/Vector.h"
#include "Types/Ant.h"

int main(int ArgumentCount, const char* ArgumentValues[]) {
	argparse::ArgumentParser Parser("Langton's ant");

	Parser.add_description("Efficient modular implementation of Langton's ant universal Turing machine which supports R45/L45, R90/L90, R135/L135, U(U turn), and C(Continue)");

	Parser.add_argument("-x")
		.default_value<SizeType>(1000)
		.help("Canvas width");
		
	Parser.add_argument("-y")
		.default_value<SizeType>(1000)
		.help("Canvas height");

	Parser.add_argument("-m")
		.default_value(std::vector<std::string>{"LRRRRRLLR"})
		.append()
		.help("Defines a state machine");
	
	Parser.add_argument("-a")
		.default_value(std::vector<std::string>{"P(500,500)D(0,-1)M(LR)"})
		.append()
		.help("Defines a ant, format: \"P(X,Y)D(DX,DY)M(M)S(S)F(W?)\" can be chained using ';'");

	Parser.add_argument("-s")
		.default_value<size_t>(1000)
		.help("Defines when to take canvas snapshots");

	Parser.add_argument("-t")
		.default_value<size_t>(1)
		.help("Defines how many encoding threads");

	Parser.add_argument("-i")
		.default_value("i5000")
		.help("Defines how many iterations should be evaluated, i50b runs for 50b iterations, t50s runs for 50 seconds.");

	Parser.add_argument("-o")
		.default_value("frame-%d.png")
		.help("Defines how to output frames");

	Parser.add_argument("-f")
		.default_value("pal")
		.help("Defines the output format, available values: pal(palette) or idx(index/grayscale)");

	try {
		Parser.parse_args(ArgumentCount, ArgumentValues);
	}
	catch (const std::exception& Error) {
		std::cerr << Error.what() << std::endl;
		std::cerr << Parser;
		std::exit(1);
	}

	SimulationState Simulation = {};

	DEBUG_PRINT(
		"Setting canvas size to %u, %u\n",
		Parser.get<SizeType>("-x"),
		Parser.get<SizeType>("-y")
	);
	
	Simulation.Resize({
		Parser.get<SizeType>("-x"),
		Parser.get<SizeType>("-y")
	});
	
	std::vector<std::vector<DirectionEnum>> StateMachines = {};
	Configs::ParserStatus Result = Configs::ParserStatus::OK;

	for (auto& String : Parser.get<std::vector<std::string>>("-m")) {
		Result = Configs::ParseStateMachines(
			String,
			StateMachines
		);

		if (Result != Configs::ParserStatus::OK) {
			std::cerr << "Failed to parse state machine \"" << String << "\" with status " << Configs::ErrorCodes[(int)Result] << '\n';
			return 1;
		}
	}

	DEBUG_PRINT("StateMachines:\n");
	for (auto& Item : StateMachines)
		DEBUG_PRINT("\t%s\n", StateMachineToString(Item).c_str());
	
	for (auto& String : Parser.get<std::vector<std::string>>("-a")) {
		Result = Configs::ParseAnts(
			String,
			StateMachines,
			Simulation.TemplateAnts
		);

		if (Result != Configs::ParserStatus::OK) {
			std::cerr << "Failed to parse ant \"" << String << "\" with status " << Configs::ErrorCodes[(int)Result] << '\n';
			return 1;
		}
	}

	Configs::Timing EvalCondition = {};
	size_t SaveIters = Parser.get<size_t>("-s");

	Result = Configs::ParseTiming(Parser.get<std::string>("-i"), &EvalCondition);

	if (Result != Configs::ParserStatus::OK) {
		std::cerr << "Failed to parse evaluation condition with status " << Configs::ErrorCodes[(int)Result] << '\n';
		return 1;
	}

	Simulation.Reset();

	size_t Iterations = 0;
	auto Start = std::chrono::high_resolution_clock::now();

	
}