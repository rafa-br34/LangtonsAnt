#pragma once
#include <iostream>
#include <cstdint>

#include "../Common.h"
#include "Vector.h"



enum class DirectionEnum : int8_t {
	R45  =  1, // Right 45°
	R90  =  2, // Right 90°
	R135 =  3, // Right 135°
	
	L45  = -1, // Left 45°
	L90  = -2, // Left 90°
	L135 = -3, // Left 135°

	C =  0, // Continue
	U = 4, // U Turn
	R = R90,
	L = L90,
};

// @note Add 4 before indexing
const char* DirectionStrings[] = {
	"L135",
	"L90",
	"L45",

	"C",

	"R45",
	"R90",
	"R135",

	"U"
};


// Directions: (360 / 8)
// 7 0 1
// 6 o 2
// 5 4 3
constexpr int8_t c_DirectionsX[] = {  0,  1,  1,  1,  0, -1, -1, -1 };
constexpr int8_t c_DirectionsY[] = { -1, -1,  0,  1,  1,  1,  0, -1 };
constexpr int8_t c_VectorLookup[] = { 7, 0, 1, 6, 0, 2, 5, 4, 3 };

template<typename CellType=uint8_t, typename SizeType=int>
class Ant {
public:
	Vector2<SizeType> LastPosition;
	Vector2<SizeType> Position;
	Vector2<int8_t> Direction;
	
	DirectionEnum* StateMachine = nullptr;
	SizeType StateMachineSize = 0;


	FORCE_INLINE void Rotate(int8_t Rotation) {
		// Decode direction vector into direction index by flattening it and indexing a lookup table,
		// add the new rotation + 8(rotation can be negative), and then mod 8
		int8_t CurrentDirection = (c_VectorLookup[(3 * this->Direction.Y + this->Direction.X) + 4] + Rotation + 8) % 8;
		
		this->Direction.X = c_DirectionsX[CurrentDirection];
		this->Direction.Y = c_DirectionsY[CurrentDirection];
	}

	FORCE_INLINE void WrapPosition(const Vector2<SizeType>& GridSize) {
		if (this->Position.X >= GridSize.X) this->Position.X = 0;
		if (this->Position.Y >= GridSize.Y) this->Position.Y = 0;
		if (this->Position.X < 0) this->Position.X = GridSize.X - 1;
		if (this->Position.Y < 0) this->Position.Y = GridSize.Y - 1;
	}

	// Single step update (used for a single ant)
	FORCE_INLINE uint8_t Update(CellType* Grid, const Vector2<SizeType>& GridSize, bool Wrap=false) {
		auto& Pos = this->Position;
		auto& Dir = this->Direction;

		// Check if the last update has landed us in a invalid position
		if (Pos.X >= GridSize.X || Pos.Y >= GridSize.Y || Pos.X < 0 || Pos.Y < 0) return 0;

		CellType* Cell = Grid + FLATTEN_2D(Pos.X, Pos.Y, GridSize.X);
		
		this->Rotate((int8_t)this->StateMachine[*Cell]);
		Pos.X += Dir.X; Pos.Y += Dir.Y;

		*Cell = (*Cell + 1) % this->StateMachineSize;
		
		if (Wrap) this->WrapPosition(GridSize);
		return 1;
	}

	// Double step update (used for multiple ants)
	FORCE_INLINE void UpdatePosition(const CellType* Grid, const Vector2<SizeType>& GridSize, bool Wrap=false) {
		auto& Last = this->LastPosition;
		auto& Pos = this->Position;
		auto& Dir = this->Direction;
		
		Last.X = Pos.X; Last.Y = Pos.Y;
		
		this->Rotate((int8_t)this->StateMachine[Grid[FLATTEN_2D(Pos.X, Pos.Y, GridSize.X)]]);
		Pos.X += Dir.X; Pos.Y += Dir.Y;

		if (Wrap) this->WrapPosition(GridSize);
	}

	FORCE_INLINE uint8_t UpdateCell(CellType* Grid, const Vector2<SizeType>& GridSize) {
		auto& Last = this->LastPosition;
		auto& Pos = this->Position;

		// Check if the last update has landed us in a invalid position
		// Positive out of bounds checks go first since a overflow will also trigger them
		if (Pos.X >= GridSize.X || Pos.Y >= GridSize.Y || Pos.X < 0 || Pos.Y < 0) return 0;

		CellType* Cell = Grid + FLATTEN_2D(Last.X, Last.Y, GridSize.X);
		*Cell = (*Cell + 1) % this->StateMachineSize;

		return 1;
	}

	Ant(SizeType X, SizeType Y, int8_t DX, int8_t DY, DirectionEnum* StateMachine, SizeType StateMachineSize) {
		this->Position.X = X;
		this->Position.Y = Y;

		this->Direction.X = DX;
		this->Direction.Y = DY;

		this->StateMachine = StateMachine;
		this->StateMachineSize = StateMachineSize;
	}

	Ant(Vector2<SizeType> Position, Vector2<int8_t> Direction, DirectionEnum* StateMachine, SizeType StateMachineSize) {
		this->Position = Position;

		this->Direction = Direction;

		this->StateMachine = StateMachine;
		this->StateMachineSize = StateMachineSize;
	}
};