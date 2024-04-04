#pragma once
#include "../Common.h"

#include <condition_variable>
#include <iostream>
#include <future>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <cmath>

#include <lodepng.h>

#include "SimulationState.h"
#include "Vector.h"
#include "Ant.h"


typedef uint32_t(* ColorDescriptor)(size_t);

class EncoderState {
private:
	std::vector<uint32_t> m_PaletteCache = {};
	size_t m_PaletteSize = 0; // @todo Improve palette generation (add caching)

	std::atomic<size_t> m_ThreadsActive = 0; // @todo std::atomic might not be required
	std::condition_variable m_ThreadsSignal = {};
	std::mutex m_ThreadsLock = {};

	void m_AcquireThread() {
		std::unique_lock<std::mutex> Lock(m_ThreadsLock);
		while (Threads > 0 && m_ThreadsActive >= Threads)
			m_ThreadsSignal.wait(Lock);
		
		m_ThreadsActive++;
		DEBUG_PRINT("Acquired thread(%d active)\n", (int)m_ThreadsActive);
	}

	void m_ReleaseThread() {
		std::unique_lock<std::mutex> Lock(m_ThreadsLock);
		m_ThreadsActive--;
		m_ThreadsSignal.notify_all();
		DEBUG_PRINT("Released thread(%d active)\n", (int)m_ThreadsActive);
	}

	ColorDescriptor m_LastProcedure = nullptr;

	void m_UpdatePaletteCache(size_t StateCount) {
		if (m_LastProcedure != Coloring) {
			m_PaletteCache.clear();
			m_LastProcedure = Coloring;
		}

		size_t CacheSize = m_PaletteCache.size();
		if (StateCount <= CacheSize) {
			if (m_PaletteCache.size() - CacheSize > CacheSize / 2)
				m_PaletteCache.resize(StateCount);
			
			return;
		}

		m_PaletteCache.reserve(StateCount);
		for (size_t s = m_PaletteCache.size(); s < StateCount; s++)
			m_PaletteCache.push_back(Coloring(s));
	}

	template<typename CellType>
	lodepng::State m_SetupEncoderState(size_t StateCount) {
		lodepng::State State = {};

		lodepng_state_init(&State);
		
		State.info_raw.colortype = LCT_PALETTE;
		State.info_raw.bitdepth = sizeof(CellType) * 8;
		State.info_png.color.colortype = LCT_PALETTE;
		State.info_png.color.bitdepth = sizeof(CellType) * 8;
		State.encoder.auto_convert = 0;

		// DEBUG_PRINT("Setting up palette with %d colors (%d bit depth):\n", (int)StateCount, (int)Depth);
		m_UpdatePaletteCache(StateCount);
		for (size_t i = 0; i < StateCount; i++) {
			uint32_t Color = m_PaletteCache[i];

			// DEBUG_PRINT(" %lu: %06X\n", (long unsigned int)i, Color & 0x00FFFFFF);
			lodepng_palette_add(
				&State.info_png.color,
				uint8_t((Color & 0xFF0000) >> 16),
				uint8_t((Color & 0x00FF00) >> 8),
				uint8_t((Color & 0x0000FF) >> 0),
				0xFF
			);
			lodepng_palette_add(
				&State.info_raw,
				uint8_t((Color & 0xFF0000) >> 16),
				uint8_t((Color & 0x00FF00) >> 8),
				uint8_t((Color & 0x0000FF) >> 0),
				0xFF
			);
		}

		return State;
	}

	template<typename CellType, typename SizeType>
	unsigned int m_EncodeBuffer(const CellType* Grid, const Vector2<SizeType>& Size, size_t StateCount, const std::string& Path) {
		std::vector<uint8_t> Buffer = {};
		unsigned int         Result = 0;
		lodepng::State       State = m_SetupEncoderState<CellType>(StateCount);

		Result = lodepng::encode(Buffer, (const unsigned char*)Grid, (unsigned int)Size.X, (unsigned int)Size.Y, State);

		if (Result) { DEBUG_PRINT("lodepng::encode -> %d\n", Result); return Result; }

		Result = lodepng::save_file(Buffer, Path);
		if (Result) { DEBUG_PRINT("lodepng::save_file -> %d\n", Result); return Result; }
		
		return Result;
	}

public:
	ColorDescriptor Coloring = [](size_t Index) -> uint32_t { auto I = uint32_t(Index); XOR_SHIFT32(I); return I * 0x9E3779B9; };
	size_t Threads = 1;


	template<typename CellType, typename SizeType>
	unsigned int EncodeSync(const SimulationState<CellType, SizeType>& State, std::string Path) {
		return m_EncodeBuffer(State.CanvasPointer, State.CanvasSize, State.PossibleStates, Path);
	}

	template<typename CellType, typename SizeType>
	unsigned int EncodeSync(const SimulationState<CellType, SizeType>& State, const Vector2<SizeType>& SectionPosition, const Vector2<SizeType>& SectionSize, std::string OutputPath) {
		std::vector<CellType> Section(SectionSize.X * SectionSize.Y);

		for (SizeType Y = 0; Y < SectionSize.Y; Y++) {
			auto RowStart = State.CanvasPointer + FLATTEN_2D(SectionPosition.X, SectionPosition.Y + Y, State.CanvasSize.X);

			std::copy(
				RowStart,
				RowStart + SectionSize.X,
				Section.begin() + SectionSize.Y * Y
			);
		}

		return m_EncodeBuffer(Section.data(), SectionSize, State.PossibleStates, OutputPath);
	}

	template<typename CellType, typename SizeType>
	void EncodeAsync(const SimulationState<CellType, SizeType>& State, std::string OutputPath) {
		auto GridCopy = std::make_shared<std::vector<CellType>>();
		auto GridSize = State.CanvasSize;
		
		GridCopy->assign(State.CanvasPointer, State.CanvasPointer + GridSize.X * GridSize.Y);

		m_AcquireThread();
		std::thread([&, GridCopy, GridSize, States = State.PossibleStates, OutputPath]() {
			m_EncodeBuffer(GridCopy->data(), GridSize, States, OutputPath);
			m_ReleaseThread();
		}).detach();
	}

	template<typename CellType, typename SizeType>
	void EncodeAsync(const SimulationState<CellType, SizeType>& State, const Vector2<SizeType>& SectionPosition, const Vector2<SizeType>& SectionSize, std::string OutputPath) {
		auto Section = std::make_shared<std::vector<CellType>>(SectionSize.X * SectionSize.Y);

		for (SizeType Y = 0; Y < SectionSize.Y; Y++) {
			auto RowStart = State.CanvasPointer + FLATTEN_2D(SectionPosition.X, SectionPosition.Y + Y, State.CanvasSize.X);

			std::copy(
				RowStart,
				RowStart + SectionSize.X,
				Section->begin() + SectionSize.Y * Y
			);
		}

		m_AcquireThread();
		std::thread([&, Section, SectionSize, States = State.PossibleStates, OutputPath]() {
			m_EncodeBuffer(Section->data(), SectionSize, States, OutputPath);
			m_ReleaseThread();
		}).detach();
	}

	void WaitJobs() {
		std::unique_lock<std::mutex> Lock(m_ThreadsLock);
		while (m_ThreadsActive > 0)
			m_ThreadsSignal.wait(Lock);
	}

	EncoderState() = default;
	~EncoderState() {
		WaitJobs();
	}
};