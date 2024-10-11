#pragma once
#include "../Common.h"

#include <cstdint>
#include <vector>
#include <string>

#include "Vector.h"

enum class DirectionEnum : int8_t {
	R45  =  1, // Right 45°
	R90  =  2, // Right 90°
	R135 =  3, // Right 135°
	
	L45  = -1, // Left 45°
	L90  = -2, // Left 90°
	L135 = -3, // Left 135°

	U = 4, // U Turn
	C = 0, // Continue
	R = R90,
	L = L90,
};

// @note Add 4 before indexing
const char* DirectionStrings[] = {
	"L135",
	"L",
	"L45",

	"C",

	"R45",
	"R",
	"R135",

	"U"
};

std::string StateMachineToString(const std::vector<DirectionEnum>& StateMachine, const char* Separator="") {
	std::string NewString = {};
	size_t Size = StateMachine.size();
	
	NewString.reserve(Size * 3);

	for (size_t i = 0; i < Size; i++)
		NewString += std::string(DirectionStrings[int8_t(StateMachine[i]) + 3]) + std::string((i + 1 != Size) ? Separator : "");

	return NewString;
}


// Angles (360 / 8) = 45°
// 7 0 1
// 6 x 2
// 5 4 3

// Lookup tables
CONSTEXPR int8_t c_DirectionsX[] = {  0,  1,  1,  1,  0, -1, -1, -1 }; // Index to X direction
CONSTEXPR int8_t c_DirectionsY[] = { -1, -1,  0,  1,  1,  1,  0, -1 }; // Index to Y direction
CONSTEXPR int8_t c_VectorLookup[] = { 7, 0, 1, 6, 0, 2, 5, 4, 3 }; // Flattened direction to rotation

class Ant {
public:
	Vector2<SizeType> LastPosition = {};
	Vector2<SizeType> Position;
	Vector2<int8_t> Direction;
	
	SizeType StepSize = 1;
	bool Wrap = false;
	
	std::vector<DirectionEnum> StateMachine = {};

	INLINE void Rotate(int8_t Rotation) {
		// Decode direction vector into direction index by flattening it (adding 4 as getting -4 is possible) and indexing a lookup table,
		// add the new rotation + 8 (rotation can be negative), and then mod 8
		int8_t CurrentDirection = (c_VectorLookup[(3 * Direction.Y + Direction.X) + 4] + Rotation + 8) % 8;
		
		Direction.X = c_DirectionsX[CurrentDirection];
		Direction.Y = c_DirectionsY[CurrentDirection];
	}

	INLINE bool ValidatePosition(const Vector2<SizeType>& GridSize) {
		// Check if the last update has landed us in a invalid position
		// Positive out of bounds checks go first since a overflow will also trigger them
		return Position.X < GridSize.X && Position.Y < GridSize.Y && Position.X >= 0 && Position.Y >= 0;
	}

	INLINE void WrapPosition(const Vector2<SizeType>& GridSize) {
		if (Position.X >= GridSize.X) Position.X = 0;
		if (Position.Y >= GridSize.Y) Position.Y = 0;
		if (Position.X < 0) Position.X = GridSize.X - 1;
		if (Position.Y < 0) Position.Y = GridSize.Y - 1;
	}

	// Single step update (used for a single ant)
	INLINE uint8_t Update(CellType* Grid, const Vector2<SizeType>& GridSize) {
		auto& Dir = Direction;
		auto& Pos = Position;

		if (!ValidatePosition(GridSize)) return 0;

		CellType* Cell = Grid + FLATTEN_2D(Pos.X, Pos.Y, GridSize.X);
		
		Rotate((int8_t)StateMachine[*Cell % StateMachine.size()]);
		Pos += Dir * StepSize;

		*Cell = (*Cell + 1) % StateMachine.size();
		
		if (Wrap) WrapPosition(GridSize);
		return 1;
	}

	// Double step update (used for multiple ants)
	INLINE void UpdatePosition(const CellType* Grid, const Vector2<SizeType>& GridSize) {
		auto& Last = LastPosition;
		auto& Dir  = Direction;
		auto& Pos  = Position;
		
		Rotate((int8_t)StateMachine[Grid[FLATTEN_2D(Pos.X, Pos.Y, GridSize.X)] % StateMachine.size()]);
		Last = Pos;
		Pos += Dir * StepSize;

		if (Wrap) WrapPosition(GridSize);
	}

	INLINE uint8_t UpdateCell(CellType* Grid, const Vector2<SizeType>& GridSize) {
		auto& Last = LastPosition;

		if (!ValidatePosition(GridSize)) return 0;

		CellType* Cell = Grid + FLATTEN_2D(Last.X, Last.Y, GridSize.X);
		*Cell = (*Cell + 1) % StateMachine.size();

		return 1;
	}

	Ant(SizeType X, SizeType Y, int8_t DX, int8_t DY, std::vector<DirectionEnum> StateMachine, bool Wrap = false, SizeType StepSize = 1)
	: Position(X, Y), Direction(DX, DY), StepSize(StepSize), Wrap(Wrap), StateMachine(StateMachine) { }

	Ant(Vector2<SizeType> Position, Vector2<int8_t> Direction, std::vector<DirectionEnum> StateMachine, bool Wrap = false, SizeType StepSize = 1)
	: Position(Position), Direction(Direction), StepSize(StepSize), Wrap(Wrap), StateMachine(StateMachine) { }

	Ant() = default;
};