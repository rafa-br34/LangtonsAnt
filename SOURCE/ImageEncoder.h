#pragma once
#include <iostream>
#include <thread>
#include <atomic>

#include "../LIBRARIES/lodepng/lodepng.h"

#include "Common.h"

#include "Types/Vector.h"



namespace ImageEncoder {
	FORCE_INLINE uint32_t GetColor(uint32_t Input) {
		XOR_SHIFT32(Input);
		return Input * 0x9E3779B9;
	}
	
	template<typename StateType=uint8_t>
	void SetupEncoderState(lodepng::State* State, StateType ColorCount) {
		// @todo Maybe we can remove this
		// lodepng_state_cleanup(State);
		// lodepng_state_init(State);

		State->info_raw.colortype = LCT_PALETTE;
		State->info_raw.bitdepth = sizeof(StateType) * 8;
		State->info_png.color.colortype = LCT_PALETTE;
		State->info_png.color.bitdepth = sizeof(StateType) * 8;
		State->encoder.auto_convert = 0;

		std::cout << "Setting up encoder palette with " << (int)ColorCount << " colors:\n";

		// This function might be called multiple times
		lodepng_palette_clear(&State->info_png.color);
		lodepng_palette_clear(&State->info_raw);

		for (StateType i = 0; i < ColorCount; i++) {
			auto C = GetColor((uint32_t)i);

			printf(" %lu: %06X\n", (long unsigned int)i, C & 0x00FFFFFF);
			lodepng_palette_add(
				&State->info_png.color,
				(C & 0xFF0000) >> 16,
				(C & 0x00FF00) >> 8,
				(C & 0x0000FF) >> 0,
				0xFF
			);
			lodepng_palette_add(
				&State->info_raw,
				(C & 0xFF0000) >> 16,
				(C & 0x00FF00) >> 8,
				(C & 0x0000FF) >> 0,
				0xFF
			);
		}
	}

	template<typename CellType, typename SizeType>
	unsigned int SaveCanvas(const CellType* Grid, const Vector2<SizeType>& GridSize, lodepng::State& EncoderState, const std::string& Path) {
		std::vector<unsigned char> FileOutput;
		unsigned int Result = 0;

		if (Result = lodepng::encode(FileOutput, Grid, GridSize.X, GridSize.Y, EncoderState))
			return Result;
		
		if (Result = lodepng::save_file(FileOutput, Path))
			return Result;

		return 0;
	}

	template<typename CellType, typename SizeType, typename CounterType>
	void SaveCanvasAsync(const CellType* Grid, const Vector2<SizeType>& GridSize, lodepng::State& EncoderState, const std::string& Path, std::atomic<CounterType>& Counter) {
		auto* GridSnapshot = new CellType[GridSize.X * GridSize.Y];
	
		memcpy(GridSnapshot, Grid, GridSize.X * GridSize.Y * sizeof(CellType));

		Counter++;
		auto NewThread = std::thread([&Counter, &EncoderState, GridSnapshot, GridSize, Path]() {
			SaveCanvas(GridSnapshot, GridSize, EncoderState, Path);
			delete[] GridSnapshot;
			Counter--;
		});
		NewThread.detach();
	}

	template<typename CellType, typename SizeType>
	unsigned int SaveCanvas(const CellType* Grid, const Vector2<SizeType>& GridSize, const Vector2<SizeType>& SectionPosition, const Vector2<SizeType>& SectionSize, lodepng::State& EncoderState, const std::string& Path) {
		CellType* SectionGrid = new CellType[SectionSize.X * SectionSize.Y];

		for (SizeType Y = 0; Y < SectionSize.Y; Y++) {
			auto GridRowStart = FLATTEN_2D(SectionPosition.X, SectionPosition.Y + Y, GridSize.X);
			auto SectionRowStart = SectionSize.Y * Y;

			memcpy(SectionGrid + SectionRowStart, Grid + GridRowStart, SectionSize.X);
			//for (SizeType X = 0; X < SectionSize.X; X++) SectionGrid[SectionRowStart + X] = Grid[GridRowStart + X];
		}

		auto Result = SaveCanvas(SectionGrid, SectionSize, EncoderState, Path);
		delete[] SectionGrid;

		return Result;
	}

	template<typename CellType, typename SizeType, typename CounterType>
	void SaveCanvasAsync(const CellType* Grid, const Vector2<SizeType>& GridSize, const Vector2<SizeType>& SectionPosition, const Vector2<SizeType>& SectionSize, lodepng::State& EncoderState, const std::string& Path, std::atomic<CounterType>& Counter) {
		CellType* SectionGrid = new CellType[SectionSize.X * SectionSize.Y];

		for (SizeType Y = 0; Y < SectionSize.Y; Y++) {
			auto GridRowStart = FLATTEN_2D(SectionPosition.X, SectionPosition.Y + Y, GridSize.X);
			auto SectionRowStart = SectionSize.Y * Y;

			memcpy(SectionGrid + SectionRowStart, Grid + GridRowStart, SectionSize.X);
			//for (SizeType X = 0; X < SectionSize.X; X++) SectionGrid[SectionRowStart + X] = Grid[GridRowStart + X];
		}

		Counter++;
		auto NewThread = std::thread([&Counter, &EncoderState, SectionGrid, SectionSize, Path]() {
			SaveCanvas(SectionGrid, SectionSize, EncoderState, Path);
			delete[] SectionGrid;
			Counter--;
		});
		NewThread.detach();
	}

}