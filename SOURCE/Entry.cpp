#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <map>

#include "CommandLineInterface.h"
#include "ImageEncoder.h"
#include "Common.h"

#include "Types/Vector.h"
#include "Types/Ant.h"




std::string StateMachineToString(const DirectionEnum* StateMachine, size_t Size, const char* Separator="") {
	std::string NewString;
	
	NewString.reserve(Size * 3);

	for (size_t i = 0; i < Size; i++)
		NewString += std::string(DirectionStrings[int8_t(StateMachine[i]) + 3]) + std::string((i + 1 != Size) ? Separator : "");

	return NewString;
}

template<typename AntType>
FORCE_INLINE void IterateAnts(std::vector<AntType>& AntList) {}


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

int main(int ArgCount, const char* Args[]) {
	using enum DirectionEnum;
	DirectionEnum StateMachine[] = {
		//R,R,L,L,L,R,L,L,L,R,R,R // Creates a filled triangle shape
		//L,L,R,R,R,L,R,L,R,L,L,R // Creates a convoluted highway
		L,R,R,R,R,R,L,L,R // Fills space in a square around itself
		//L,L,R,R // Grows symmetrically
		//R,L,R // Grows chaotically
		
		//R,R,L,L,R,L,L,L,R
		//R,U,R,L,R,U
		//R,L,L,L,R,L,R,R,L,L,R,R,R // Similar to LRRRRRLLR but halts growth by iteration 8477782376

		//R,L // Default Langton's ant
	};

	size_t StateMachineSize = sizeof(StateMachine) / sizeof(DirectionEnum);


	Vector2<int> CanvasSize = {1080, 1080};//{ 30720, 17280 };

	uint8_t* CanvasPointer = new uint8_t[CanvasSize.X * CanvasSize.Y]{ 0 };


	std::cout << "State machine: " << StateMachineToString(StateMachine, StateMachineSize, "") << '\n';

	lodepng::State EncoderState;
	ImageEncoder::SetupEncoderState<uint8_t>(&EncoderState, StateMachineSize);
	
	auto Center = CanvasSize / Vector2(2, 2);

	/*
	std::vector<Ant<uint8_t>> Ants = {};
	Ants.push_back(Ant<uint8_t>(Center + Vector2(0, 10), Vector2<int8_t>(0, -1), StateMachine, StateMachineSize));
	Ants.push_back(Ant<uint8_t>(Center - Vector2(0, 10), Vector2<int8_t>(0,  1), StateMachine, StateMachineSize));

	Ants.push_back(Ant<uint8_t>(Center + Vector2(10, 0), Vector2<int8_t>(-1, 0), StateMachine, StateMachineSize));
	Ants.push_back(Ant<uint8_t>(Center - Vector2(10, 0), Vector2<int8_t>( 1, 0), StateMachine, StateMachineSize));
	//*/
	
	auto AntObject = Ant<uint8_t>(Center, Vector2<int8_t>(0, -1), StateMachine, StateMachineSize);

	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 output.mp4
	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 -s 1920x1920 output.mp4
	// ffmpeg -r 60 -i "Frames/%d.png" -b:v 5M -c:v libx264 -preset veryslow -qp 0 -s 1920x1920 -sws_flags neighbor output.mp4
	// ffmpeg -r 30 -i "Frames/%d.png" -c:v libx264 -preset veryslow -qp 0 -s 7680x4320 output.mp4

	double Iterations = 21000; // Iterations 50000000000 (50b) 5000000000 (5b) 500000000 (500m) 50000000 (50m)
	double Time = 50.0; // Video time
	double Rate = 1.0; // Video frame rate
	
	auto CurrentSize = Vector2(CanvasSize.X, CanvasSize.Y);//Vector2(CanvasSize.X, CanvasSize.Y);
	std::atomic<size_t> Running = 0;
	size_t CaptureDelta = size_t(Iterations / (Time * Rate));
	for (size_t i = 0; i < size_t(Iterations); i++) {

		/*
		bool Stop = false;
		for (auto& AntObject : Ants) AntObject.UpdatePosition(CanvasPointer, CanvasSize, false);
		for (auto& AntObject : Ants) if (!AntObject.UpdateCell(CanvasPointer, CanvasSize)) { Stop = true; break; }
		//*/
		
		if (!AntObject.Update(CanvasPointer, CanvasSize)) { std::cout << "Ant out of bounds at i:" << i << '\n'; break; }
		//std::cout << "i:" << i << " x:" << AntObject.Position.X << " y:" << AntObject.Position.Y << '\n';
		
		/*
		bool B0 = AntObject.Position.X > (CanvasSize.X / 2) + (CurrentSize.X / 2) || AntObject.Position.Y > (CanvasSize.Y / 2) + (CurrentSize.Y / 2);
		bool B1 = AntObject.Position.X < (CanvasSize.X / 2) - (CurrentSize.X / 2) || AntObject.Position.Y < (CanvasSize.Y / 2) - (CurrentSize.Y / 2);
		if (B0 + B1) {
			//std::cout << "x:" << (int)CurrentSize.X << " y:" << (int)CurrentSize.Y << " i:" << i << '\n';
			CurrentSize.X += 1;//CanvasSize.X / 100;
			CurrentSize.Y += 1;//CanvasSize.Y / 100;
		}
		//*/

		/*
		if (i % 1000000 == 0) { // 1000000000 (1b) 100000000 (100m) 1000000 (1m)
			std::cout << "Encoding canvas state i:" << i << "\n";
			
			while (Running >= 2) {}
			ImageEncoder::SaveCanvasAsync(
				CanvasPointer,
				CanvasSize,
				EncoderState,
				"Frames/STATE_" + std::to_string(i/1000000) + ".png",
				Running
			);
		}
		//*/

		///*
		if (i % CaptureDelta == 0) {
			std::cout << "Capturing frame i:" << i << " f:" << i / CaptureDelta << '\n';

			while (Running > 6) {}
			/*
			ImageEncoder::SaveCanvasAsync(
				CanvasPointer,
				CanvasSize,
				Vector2((CanvasSize.X / 2) - (CurrentSize.X / 2), (CanvasSize.Y / 2) - (CurrentSize.Y / 2)),
				CurrentSize,
				EncoderState,
				"Frames/" + std::to_string(i / CaptureDelta) + ".png",
				Running
			);
			//*/
			///*
			ImageEncoder::SaveCanvasAsync(
				CanvasPointer,
				CanvasSize,
				EncoderState,
				"Frames/" + std::to_string(i / CaptureDelta) + ".png",
				Running
			);
			//*/
		}
		//*/

		//if (Stop) { std::cout << "Stopping at i:" << i << '\n'; break; }
	}

	std::cout << "Encoding final canvas state...\n";
	ImageEncoder::SaveCanvas(
		CanvasPointer,
		CanvasSize,
		EncoderState,
		"Frames/FINAL.png"
	);

	std::cout << "Waiting for encoder threads to stop...\n";
	while (Running > 0) {}

	delete[] CanvasPointer;
}