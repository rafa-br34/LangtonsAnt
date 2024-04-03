#pragma once
#include "../Common.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <lodepng.h>

#include "Vector.h"
#include "Ant.h"

template<typename CellType = CELL_TYPE, typename SizeType = SIZE_TYPE>
class SimulationState {
private:
	void m_Deallocate() {
		if (CanvasPointer) {
			delete[] CanvasPointer;
			CanvasPointer = nullptr;
		}
	}

	void m_Allocate() {
		if (!CanvasPointer)
			CanvasPointer = new CellType[CanvasSize.X * CanvasSize.Y]{};
	}
public:
	std::vector<Ant<CellType, SizeType>> TemplateAnts = {};
	std::vector<Ant<CellType, SizeType>> Ants = {};

	uint8_t*          CanvasPointer = nullptr;
	Vector2<SizeType> CanvasSize = { 0, 0 };

	CellType PossibleStates = 0;

	void AddAnt(const Ant<CellType, SizeType>& AntObject) {
		TemplateAnts.push_back(AntObject);
		PossibleStates = std::max(PossibleStates, (CellType)AntObject.StateMachine.size());
	}

	void UpdateStats() {
		PossibleStates = 0;

		for (auto& AntObject : TemplateAnts)
			PossibleStates = std::max(PossibleStates, (CellType)AntObject.StateMachine.size());
	}

	void Resize(const Vector2<SizeType>& Size) {
		if (CanvasSize != Size)
			m_Deallocate();
		
		CanvasSize = Size;
		m_Allocate();
	}

	size_t Simulate(size_t Iterations = 1) {
		if (Ants.empty()) return 0;
		
		size_t Iterated = 0;

		if (Ants.size() == 1) {
			for (; Iterated < Iterations; Iterated++) {
				if (!Ants[0].Update(CanvasPointer, CanvasSize)) { Ants.clear(); break; }
			}
		}
		else {
			for (; Iterated < Iterations; Iterated++) {
				// Update positions
				for (auto& AntObject : Ants) AntObject.UpdatePosition(CanvasPointer, CanvasSize);
				// Update cells & Remove dead ants
				Ants.erase(
					std::remove_if(
						Ants.begin(),
						Ants.end(),
						[&](auto& AntObject) { return !AntObject.UpdateCell(CanvasPointer, CanvasSize); }
					),
					Ants.end()
				);

				if (Ants.empty()) break;
			}
		}

		return Iterated;
	}
	
	void Reset(bool ResetCanvas = true, bool ResetAnts = true) {
		m_Allocate();

		if (ResetCanvas) {
			memset(CanvasPointer, 0, CanvasSize.X * CanvasSize.Y);
		}
		if (ResetAnts) {
			Ants.clear();
			Ants.insert(Ants.end(), TemplateAnts.begin(), TemplateAnts.end());
		}
	}

	SimulationState() = default;
	SimulationState(const Vector2<SizeType>& Size) { Resize(Size); }

	~SimulationState() { m_Deallocate(); }
};
