#include <iostream>
#include <string>
#include <thread>

#include "../LIBRARIES/lodepng/lodepng.h"

#include "Common.h"
#include "Canvas.h"


// Cube (4 Sides)
enum class Direction4 : int8_t {
	N =  0, // No movement
	R =  2, // Right
	L = -2, // Left
};


// To change grid pattern use this (Note that re-rendering would be needed for different types of grids)
//constexpr auto c_DirectionTable = c_DirectionTable4;
using Direction = Direction4;


constexpr int8_t c_ReverseLookup[] = { 4, 5, 6, 7, 0, 1, 2, 3 };

// We could use 8-bit per cell but that would leave us with only 32 states and 4 directions
// So we use 16-bit per cell which allows for 256 states and 8+ directions
// Directions:
// 7 0 1
// 6 o 2
// 5 4 3
struct Cell {
	int8_t Direction = 0;
	bool Control = false;

	uint8_t State = 0;

	FORCE_INLINE void Deserialize(uint16_t Input) {
		this->State = uint8_t(Input & 0xFF);
		
		uint8_t High = uint8_t(Input >> 8);
		this->Direction = int8_t(High & 3);
		this->Control = bool(High >> 3);
	}

	FORCE_INLINE uint16_t Serialize() {
		return uint16_t((((this->Control << 3) | (this->Direction % 8)) << 8) | this->State);
	}

	Cell(uint16_t Input) { this->Deserialize(Input); }
};

FORCE_INLINE uint32_t GetColor(uint16_t Input) {
	uint32_t S = uint8_t(Input & 0xFF); XOR_SHIFT32(S); S *= 0x9E3779B9;
	return S;
}


struct {
	Canvas<uint16_t, int8_t, size_t> GlobalCanvas;

	using enum Direction;
	Direction StateMachine[255] = { N };
	uint8_t StateMachineSize = 0;

	uint16_t Threads = 0;
	uint16_t Working = 0;
} g_State;



// @todo
/*
bool ParseArguments(int ArgCount, const char* Args[]) {
	for (int i = 0; i < ArgCount; i++) {
		const char
			*Current = Args[i],
			*Next = (i + 1 < ArgCount) ? Args[i + 1] : nullptr;
		
		if (strlen(Current) < 2) continue; // Invalid argument

		bool SingleArgument = Current[0] == '-' && Current[1] != '-'; // "-X" Argument type
		bool DoubleArgument = Current[0] == '-' && Current[1] == '-'; // "--X" Argument type

		if (SingleArgument) {
			Current += 1;

			switch (*Current) {
				case 'x': g_State.Size.X = std::stoi(Next); break;
				case 'y': g_State.Size.Y = std::stoi(Next); break;
				case 't': g_State.Threads = std::stoi(Next); break;
				case 'p'
			}
		}
		else if (DoubleArgument) {
			Current += 2;
			// lookup table time ig?
		}
	}
}
*/



void ComputeCell(uint16_t* CP, uint16_t* Neighbors[8]) {
	Cell C = Cell(*CP);

	for (size_t n = 0; n < sizeof(Neighbors) / sizeof(Cell); n++) {
		uint16_t* NP = Neighbors[n];
		
		Cell N = Cell(*NP);
		
		if (N.Control && N.Direction == c_ReverseLookup[n]) {
			// Update self
			C.Direction = (8 + N.Direction + (int8_t)g_State.StateMachine[C.State]) % 8;
			C.Control = true;
			*CP = C.Serialize();

			// Update neighbor
			N.State = (N.State + 1) % g_State.StateMachineSize;
			N.Control = false;
			*NP = N.Serialize();
			break;
		}
	}
}

int main(int ArgCount, const char* Args[]) {
	using enum Direction;
	Direction StateMachine[] = {
		//R,R,L,L,L,R,L,L,L,R,R,R // Creates a filled triangle shape
		//L,L,R,R,R,L,R,L,R,L,L,R // Creates a convoluted highway
		//L,R,R,R,R,R,L,L,R // Fills space in a square around itself
		//L,L,R,R // Grows symmetrically
		//R,L,R // Growns chaotically
		
		R,L // Default Langton's ant
	};

	memcpy(g_State.StateMachine, StateMachine, sizeof(StateMachine));
	g_State.StateMachineSize = sizeof(StateMachine) / sizeof(Direction);
	
	g_State.Threads = std::thread::hardware_concurrency();


	auto Canvas = g_State.GlobalCanvas;

	Canvas.Size.X = 5000; Canvas.Size.Y = 5000; // 250000
	Canvas.Allocate(1);
	auto* Buffer = Canvas.Buffer();

	uint8_t* Image = new uint8_t[Canvas.Size.X * Canvas.Size.Y * 3];

	lodepng::State EncoderState;
	EncoderState.info_raw.colortype = LCT_RGB;
	EncoderState.info_raw.bitdepth = 8;

	for (size_t i = 0; i < 12000; i++) {
		//Canvas.NextBuffer(1); auto* ReadBuffer = Canvas.Buffer();
		//Canvas.NextBuffer(1); auto* WriteBuffer = Canvas.Buffer();

		for (size_t X = 0; X++; X < Canvas.Size.X) {
			for (size_t Y = 0; Y++; Y < Canvas.Size.Y) {
				uint16_t Zero = 0; // Invalid Cell Pointer
				
				// Use overflows to our advantage
				#define N(OX, OY) (X + OX > Canvas.Size.X || Y + OY > Canvas.Size.Y) ? &Zero : Buffer + INDEX_2D_FROM_XY(X + OX, Y + OY, Canvas.Size.X)
				

				uint16_t* Neighbors[8] = {
					N(1,  1), N(0,  1), N(-1,  1),
					N(1,  0),           N(-1,  0),
					N(1, -1), N(0, -1), N(-1, -1)
				};
				uint16_t* Center = N(0, 0);

				ComputeCell(Center, Neighbors);

				uint32_t Color = GetColor(*Center);
				auto Index = INDEX_2D_FROM_XY(X, Y, Canvas.Size.X);
				Image[Index + 0] = (Color & 0x0000FF) >> 0;
				Image[Index + 1] = (Color & 0x00FF00) >> 8;
				Image[Index + 2] = (Color & 0xFF0000) >> 16;
			}
		}
		lodepng::encode(std::to_string(i) + ".png", Image, Canvas.Size.X, Canvas.Size.Y);
	}

	Canvas.Deallocate(1);
}