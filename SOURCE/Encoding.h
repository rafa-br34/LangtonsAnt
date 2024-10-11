#pragma once
#include "Common.h"

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

#include "Types/SimulationState.h"
#include "Types/Vector.h"
#include "Types/Ant.h"

namespace Encoding {
	union RGBA32 {
		uint32_t Color = 0;

		struct {
			uint8_t R, G, B, A;
		} RGBA;

		operator uint32_t() const { return Color; }
	};

	typedef RGBA32(* ColorDescriptor)(size_t);

	class PaletteManager {
	private:
		std::vector<RGBA32> m_Palette = {};
		ColorDescriptor m_LastProcedure = nullptr;

	public:
		ColorDescriptor ColoringProcedure = [](size_t Index) -> RGBA32 {
			auto I = uint32_t(Index);
			XOR_SHIFT32(I);
			I *= 0x9E3779B9;
			return { ((I & 0x00FF0000) >> 16) | (I & 0x0000FF00) | ((I & 0x000000FF) << 16) | (I & 0xFF000000) };
		};

		void ResizePalette(size_t StateCount) {
			if (m_LastProcedure != ColoringProcedure) { // Reset the cache if the coloring procedure has changed
				m_LastProcedure = ColoringProcedure;
				m_Palette.clear();
			}

			size_t CacheSize = m_Palette.size();
			if (StateCount <= CacheSize) {
				if (CacheSize - StateCount > CacheSize * 2) // Resize the cache if it's more than 2x bigger
					m_Palette.resize(StateCount);
				
				return;
			}

			m_Palette.reserve(StateCount);
			for (size_t s = m_Palette.size(); s < StateCount; s++)
				m_Palette.push_back(ColoringProcedure(s));
		}

		INLINE size_t GetSize() const {
			return m_Palette.size();
		}

		INLINE RGBA32 GetColor(size_t Index) {
			if (Index + 1 > m_Palette.size())
				ResizePalette(Index + 1);

			return m_Palette[Index];
		}

		INLINE RGBA32* GetData() {
			return m_Palette.data();
		}

		INLINE RGBA32 operator[](size_t Index) {
			return m_Palette[Index];
		}
	};

	class ThreadManager {
	private:
		std::condition_variable m_ThreadsSignal = {};
		std::atomic<size_t>     m_ThreadsActive = 0; // @todo std::atomic might not be required because of m_ThreadsLock and m_ThreadsSignal
		std::mutex              m_ThreadsLock = {};

	public:
		size_t ThreadCount = 0;

		void AcquireThread() {
			std::unique_lock<std::mutex> Lock(m_ThreadsLock);
			while (ThreadCount > 0 && m_ThreadsActive >= ThreadCount)
				m_ThreadsSignal.wait(Lock);
			
			m_ThreadsActive++;
			DEBUG_PRINT("Acquired thread (%d active)\n", (int)m_ThreadsActive);
		}

		void ReleaseThread() {
			std::unique_lock<std::mutex> Lock(m_ThreadsLock);
			m_ThreadsActive--;
			m_ThreadsSignal.notify_all();
			DEBUG_PRINT("Released thread (%d active)\n", (int)m_ThreadsActive);
		}

		void WaitJobs() {
			std::unique_lock<std::mutex> Lock(m_ThreadsLock);
			while (m_ThreadsActive > 0)
				m_ThreadsSignal.wait(Lock);
		}

		size_t ActiveThreads() const { return m_ThreadsActive; }

		ThreadManager() = default;
		~ThreadManager() { WaitJobs(); }
	};

	enum class ImageFormat : uint8_t {
		PNG_PALETTE,
		PNG_GRAYSCALE,
	};

	class EncoderState {
	private:
		lodepng::State m_SetupEncoderState(size_t StateCount, unsigned int BitDepth) {
			lodepng::State State = {};

			lodepng_state_init(&State);

			LodePNGColorType ColorType;
			
			switch (Format) {
				case ImageFormat::PNG_PALETTE:   ColorType = LCT_PALETTE; break;
				case ImageFormat::PNG_GRAYSCALE: ColorType = LCT_GREY; break;

				default: {
					DEBUG_PRINT("Unknown data format %d, ColorType will be set to LCT_GREY\n", (int)Format);
					ColorType = LCT_GREY;
					break;
				}
			}

			State.info_png.color.colortype = ColorType;
			State.info_raw.colortype = ColorType;
			
			State.info_png.color.bitdepth = BitDepth;
			State.info_raw.bitdepth = BitDepth;
			
			State.encoder.auto_convert = 0;

			if (Format != ImageFormat::PNG_PALETTE) return State;

			DEBUG_PRINT("Setting up palette with %d colors (%d bits per pixel):\n", (int)StateCount, (int)BitDepth);
			Palette.ResizePalette(StateCount);
			for (size_t i = 0; i < StateCount; i++) {
				auto [R, G, B, A] = Palette[i].RGBA;

				DEBUG_PRINT(" %lu: %06X\n", (long unsigned int)i, Palette[i].Color & 0x00FFFFFF);
				lodepng_palette_add(&State.info_png.color, R, G, B, 0xFF);
				lodepng_palette_add(&State.info_raw, R, G, B, 0xFF);
			}

			return State;
		}


		template<typename Function>
		void m_EncodeBuffer(const CellType* Grid, const Vector2<SizeType>& Size, size_t StateCount, Function Callback) {
			std::vector<uint8_t> ImageData = {};
			lodepng::State State = m_SetupEncoderState(StateCount, 8);
			
			unsigned int Result = lodepng::encode(ImageData, (const unsigned char*)Grid, (unsigned int)Size.X, (unsigned int)Size.Y, State);

			ASSERT_MSG(Result == 0, "lodepng::encode -> %d\n", Result);

			Callback(ImageData, Vector2<int>(Size.X, Size.Y), Result);
		}

	public:
		PaletteManager Palette = {};
		ThreadManager  Threads = {};
		
		ImageFormat Format = ImageFormat::PNG_PALETTE;

		template<typename Function>
		void EncodeSync(const SimulationState& State, Function Callback) {
			m_EncodeBuffer(State.CanvasPointer, State.CanvasSize, State.PossibleStates, Callback);
		}

		template<typename Function>
		void EncodeSync(const SimulationState& State, const Vector2<SizeType>& SectionPosition, const Vector2<SizeType>& SectionSize, Function Callback) {
			std::vector<CellType> Section(SectionSize.X * SectionSize.Y);

			for (SizeType Y = 0; Y < SectionSize.Y; Y++) {
				auto RowStart = State.CanvasPointer + FLATTEN_2D(SectionPosition.X, SectionPosition.Y + Y, State.CanvasSize.X);

				std::copy(
					RowStart,
					RowStart + SectionSize.X,
					Section.begin() + SectionSize.Y * Y
				);
			}

			return m_EncodeBuffer(Section.data(), SectionSize, State.PossibleStates, Callback);
		}

		template<typename Function>
		void EncodeAsync(const SimulationState& State, Function Callback) {
			auto GridCopy = std::make_shared<std::vector<CellType>>();
			auto GridSize = State.CanvasSize;
			
			GridCopy->assign(State.CanvasPointer, State.CanvasPointer + GridSize.X * GridSize.Y);

			Threads.AcquireThread();
			std::thread([&, GridCopy, GridSize, States = State.PossibleStates, Callback]() {
				m_EncodeBuffer(GridCopy->data(), GridSize, States, Callback);
				Threads.ReleaseThread();
			}).detach();
		}

		template<typename Function>
		void EncodeAsync(const SimulationState& State, const Vector2<SizeType>& SectionPosition, const Vector2<SizeType>& SectionSize, Function Callback) {
			auto Section = std::make_shared<std::vector<CellType>>(SectionSize.X * SectionSize.Y);

			for (SizeType Y = 0; Y < SectionSize.Y; Y++) {
				auto RowStart = State.CanvasPointer + FLATTEN_2D(SectionPosition.X, SectionPosition.Y + Y, State.CanvasSize.X);

				std::copy(
					RowStart,
					RowStart + SectionSize.X,
					Section->begin() + SectionSize.Y * Y
				);
			}

			Threads.AcquireThread();
			std::thread([&, Section, SectionSize, States = State.PossibleStates, Callback]() {
				m_EncodeBuffer(Section->data(), SectionSize, States, Callback);
				Threads.ReleaseThread();
			}).detach();
		}

		EncoderState() = default;
		~EncoderState() { Threads.WaitJobs(); }
	};
}
