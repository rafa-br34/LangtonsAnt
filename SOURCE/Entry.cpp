#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <map>

#include <array>

#include <argparse/argparse.hpp>

#include "ConfigParser.h"
#include "Encoding.h"
#include "Common.h"


#include "Types/SimulationState.h"
#include "Types/Vector.h"
#include "Types/Ant.h"


/* @todo argument parser yay
template<typename Type, typename SizeType=size_t>
void BuildPossibilityList(std::vector<std::vector<Type>>& List, SizeType Size, Type* Dictionary, SizeType DictionarySize) {
	Type* Current = new Type[Size];
	SizeType* Counters = new SizeType[Size]{ 0 };

	for (SizeType i = 0; i < Size; i++) Current[i] = Dictionary[0];

	while (Counters[Size - 1] < DictionarySize) {
		
		for (SizeType i = 0; i < Size; i++) Current[i] = Dictionary[Counters[i]];
		List.push_back(std::vector<Type>(Current, Current + Size));

		Counters[0]++;
		for (SizeType i = 0; i < Size; i++) {
			if (i + 1 < Size && Counters[i] >= DictionarySize) {
				Counters[i] = 0;
				Counters[i + 1]++;
			}
		}
	}

	delete[] Counters;
	delete[] Current;
}


void ParseArguments(int Count, const char* Values[], State<>& GlobalState) {
	argparse::ArgumentParser Program("Langton's ant");

	Program.add_description("Efficient modular implementation of Langton's ant universal Turing machine which supports R45/L45, R90/L90, R135/L135, U(U turn), and C(Continue)");

	Program.add_argument("-x").default_value(1000).help("Canvas width");
	Program.add_argument("-y").default_value(1000).help("Canvas height");

	Program.add_argument("-m")
		.default_value(std::vector<std::string>{"LRRRRRLLR"})
		.append()
		.help("Defines a state machine");
	
	Program.add_argument("-a")
		.default_value(std::vector<std::string>{"P(500,500)D(0,-1)M(LR)"})
		.append()
		.help("Defines a ant, format: \"P(X,Y)D(DX,DY)M(M)S(S)F(W?)\" can be chained using ';'");

	Program.add_argument("-i")
		.default_value("1000")
		.help("Defines when to take canvas snapshots");

	Program.add_argument("-s")
		.default_value("i5000")
		.help("Defines how many iterations should be evaluated, i50b runs for 50b iterations, t50 runs for 50 seconds.");


	try {
		Program.parse_args(Count, Values);
	}
	catch (const std::exception& Error) {
		std::cerr << Error.what() << std::endl;
		std::cerr << Program;
		std::exit(1);
	}

	GlobalState.CanvasSize.X = Program.get<int>("-x");
	GlobalState.CanvasSize.Y = Program.get<int>("-y");
	
	std::vector<std::vector<DirectionEnum>> StateMachines = {};

	ConfigParser::ParseStateMachines(Program.get<std::string>("-m"), StateMachines);
	ConfigParser::ParseAnts(Program.get<std::string>("-a"), StateMachines, GlobalState.TemplateAnts);

	GlobalState.Reset();
}

int main(int ArgCount, const char* ArgValues[]) {
	State GlobalState;

	ParseArguments(ArgCount, ArgValues, GlobalState);

}
*/


int main(int ArgCount, const char* ArgValues[]) {
	using enum DirectionEnum;
	std::vector<DirectionEnum> StateMachine = {
		//R,R,L,L,L,R,L,L,L,R,R,R // Creates a filled triangle shape
		//L,L,R,R,R,L,R,L,R,L,L,R // Creates a convoluted highway
		//L,R,R,R,R,R,L,L,R // Fills space in a square around itself
		//L,L,R,R // Grows symmetrically
		//R,L,R // Grows chaotically
		
		//R,R,L,L,R,L,L,L,R
		//R,U,R,L,R,U
		//R,L,L,L,R,L,R,R,L,L,R,R,R // Similar to LRRRRRLLR but halts growth by iteration 8477782376

		//R,L,U,L,R,L,R,R,L,L,R,R,R
		
		//L45,R45
		//L45,U,R45,U
		//L45,R45,L45,U,R45,U,L45,U,R45,U
		
		//R,L // Default Langton's ant
		//U,C,L45,R45
		//C,U,U,U,C,L45,R45,U,C,L45,R135
		
	};

	//std::cout << "State machine: " << StateMachineToString(StateMachine, "") << '\n';

	Vector2<int> CanvasSize(30720, 30720);//{ 30720, 17280 };

	SimulationState<uint8_t, int> Simulation = {};
	Encoding::PaletteManager Palette = {};
	Encoding::ThreadManager Threads = {};

	Threads.ThreadCount = 50;
	
	Simulation.Resize(CanvasSize);
	
	/*
	std::vector<Ant<uint8_t>> Ants = {};
	Ants.push_back(Ant<uint8_t>(Center + Vector2(0, 10), Vector2<int8_t>(0, -1), StateMachine, StateMachineSize));
	Ants.push_back(Ant<uint8_t>(Center - Vector2(0, 10), Vector2<int8_t>(0,  1), StateMachine, StateMachineSize));

	Ants.push_back(Ant<uint8_t>(Center + Vector2(10, 0), Vector2<int8_t>(-1, 0), StateMachine, StateMachineSize));
	Ants.push_back(Ant<uint8_t>(Center - Vector2(10, 0), Vector2<int8_t>( 1, 0), StateMachine, StateMachineSize));
	//*/
	auto Center = CanvasSize / Vector2(2, 2);
	//Simulation.AddAnt(Ant<uint8_t>(Center, Vector2<int8_t>(0, -1), {R,L}, true));

	//Simulation.AddAnt(Ant<uint8_t>(Center, Vector2<int8_t>(0, -1), {C,C,C,C,C,C,U,C,C,C,C,C,R45,R,R45,L135,U,C,U,L,L,R135,L45,R45,R,R135,L45,R,R,R45}, true));
	
	Simulation.AddAnt(Ant<uint8_t>(Center, Vector2<int8_t>(0, 1), {R,L,R,R,L,R,R,L,R,R,L,R}, true));
	Simulation.AddAnt(Ant<uint8_t>(Center, Vector2<int8_t>(0, 1), {R,L,C}, true));

	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 output.mp4
	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 -s 1920x1920 output.mp4
	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 -s 1920x1920 -sws_flags neighbor output.mp4
	// ffmpeg -r 30 -i "Frames/%d.png" -c:v libx264 -preset veryslow -qp 0 -s 7680x4320 output.mp4
	// ./LangtonsAnt | ~/ffmpeg/ffmpeg -y -f rawvideo -pix_fmt rgb24 -s 30720x30720 -r 30 -i - -c:v libx264 -preset veryslow -s 7680x7680 output.h264
	// ./LangtonsAnt | ~/ffmpeg/ffmpeg -y -f rawvideo -pix_fmt rgb24 -s 1920x1920 -r 30 -i - -c:v libx264 -preset veryslow -s 1920x1920 output.h264
	
	// 1ull * 1000000000ull 1b
	// 1ull * 1000000ull 1m

	size_t Iterations = 3ull * 1000000000ull;
	double FrameRate = 30.0; // Video frame rate
	double Time = 240.0; // Video time
	size_t Frames = size_t(Time * FrameRate);
	
	size_t CaptureDelta = size_t(double(Iterations) / double(Frames));

	Simulation.Reset();
	Palette.ResizePalette(Simulation.PossibleStates);

	std::mutex Mutex = {};
	size_t CurrentFrame = 0;

	for (size_t i = 0; i < Frames; i++) {
		Simulation.Simulate(CaptureDelta);//std::cout << "i:" << i << ' ' << Simulation.Simulate(CaptureDelta) << '/' << CaptureDelta << '\n';
		
		auto GridCopy = std::make_shared<std::vector<uint8_t>>();
		
		GridCopy->assign(Simulation.CanvasPointer, Simulation.CanvasPointer + CanvasSize.X * CanvasSize.Y);

		Threads.AcquireThread();
		std::thread([&, GridCopy, i]() {
			uint8_t* Canvas = GridCopy->data();

			uint8_t* Image = new uint8_t[Center.X * Center.Y * 3];

			auto Start = std::chrono::high_resolution_clock::now();
			for (int X = 0; X < Center.X; X++) {
				for (int Y = 0; Y < Center.Y; Y++) {
					//*
					auto C00 = Palette[Canvas[FLATTEN_2D(X * 2 + 0, Y * 2 + 0, CanvasSize.X)]].RGBA;
					auto C01 = Palette[Canvas[FLATTEN_2D(X * 2 + 0, Y * 2 + 1, CanvasSize.X)]].RGBA;
					auto C10 = Palette[Canvas[FLATTEN_2D(X * 2 + 1, Y * 2 + 0, CanvasSize.X)]].RGBA;
					auto C11 = Palette[Canvas[FLATTEN_2D(X * 2 + 1, Y * 2 + 1, CanvasSize.X)]].RGBA;

					size_t Index = FLATTEN_2D(X, Y, Center.X) * 3;

					Image[Index + 0] = uint8_t(((float)C00.R + (float)C01.R + (float)C10.R + (float)C11.R) / 4.f);
					Image[Index + 1] = uint8_t(((float)C00.G + (float)C01.G + (float)C10.G + (float)C11.G) / 4.f);
					Image[Index + 2] = uint8_t(((float)C00.B + (float)C01.B + (float)C10.B + (float)C11.B) / 4.f);
					//*/
				}
			}
			auto Elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - Start);

			while (CurrentFrame != i)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));

			Mutex.lock();
			std::cerr << "Frame " << CurrentFrame << " Took " << Elapsed << " seconds\n";
			fwrite(Image, 1, Center.X * Center.Y * 3, stdout);
			CurrentFrame++;
			Mutex.unlock();
			
			delete[] Image;

			Threads.ReleaseThread();
		}).detach();
		
		
		//Encoder.EncodeAsync(Simulation, std::string("Frames/") + std::to_string(i) +".png");
	}
}
