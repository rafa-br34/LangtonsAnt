// Quite literally a JS rewrite of /SOURCE/Types/Ant.h

const c_DirectionEnum = Object.freeze({
	R45:   1, // Right 45°
	R90:   2, // Right 90°
	R135:  3, // Right 135°
	
	L45:  -1, // Left 45°
	L90:  -2, // Left 90°
	L135: -3, // Left 135°

	C: 0, // Continue
	U: 4, // U Turn
	
	R:  2,
	L: -2,
})

const c_DirectionStrings = Object.freeze([
	"L135",
	"L",
	"L45",

	"C",

	"R45",
	"R",
	"R135",

	"U"
])

function CreateStateMachine(Positions) {
	let NewMachine = []
	
	for (let Name of Positions)
		NewMachine.push(c_DirectionEnum[Name])
	
	return NewMachine
}

function StateMachineToString(StateMachine, Separator="") {
	let NewString = ""

	for (Direction of StateMachine)
		NewString += c_DirectionStrings[(Direction + 3) % c_DirectionStrings.length] + Separator

	return NewString
}

// Directions: (360 / 8)
// 7 0 1
// 6 o 2
// 5 4 3
const c_DirectionsX = [  0,  1,  1,  1,  0, -1, -1, -1 ]
const c_DirectionsY = [ -1, -1,  0,  1,  1,  1,  0, -1 ]
const c_VectorLookup = [ 7, 0, 1, 6, 0, 2, 5, 4, 3 ]

class Ant {
	LastPosition = { X: 0, Y: 0 }
	Direction = { X: 0, Y: 0 }
	Position = { X: 0, Y: 0 }

	StateMachine = []

	Rotate(Rotation) {
		// Decode direction vector into direction index by flattening it and indexing a lookup table,
		// add the new rotation + 8(rotation can be negative), and then mod 8
		let CurrentDirection = (c_VectorLookup[(3 * this.Direction.Y + this.Direction.X) + 4] + Rotation + 8) % 8
		
		this.Direction.X = c_DirectionsX[CurrentDirection]
		this.Direction.Y = c_DirectionsY[CurrentDirection]
	}

	WrapPosition(GridSize) {
		if (this.Position.X < 0) this.Position.X = GridSize.X - 1
		if (this.Position.Y < 0) this.Position.Y = GridSize.Y - 1
		if (this.Position.X >= GridSize.X) this.Position.X = 0
		if (this.Position.Y >= GridSize.Y) this.Position.Y = 0
	}

	Update(Grid, GridSize, Wrap=false) {
		let Dir = this.Direction
		let Pos = this.Position

		// Check if the last update has landed us in a invalid position
		if (Pos.X >= GridSize.X || Pos.Y >= GridSize.Y || Pos.X < 0 || Pos.Y < 0) return 0

		let CellIndex = GridSize.X * Pos.Y + Pos.X
		let CellValue = Grid[CellIndex]
		
		this.Rotate(this.StateMachine[CellValue % this.StateMachine.length])
		Pos.X += Dir.X
		Pos.Y += Dir.Y

		Grid[CellIndex] = (CellValue + 1) % this.StateMachine.length
		
		if (Wrap) this.WrapPosition(GridSize)
		return 1
	}

	// Double step update (used for multiple ants)
	UpdatePosition(Grid, GridSize, Wrap=false) {
		let Last = this.LastPosition
		let Pos = this.Position
		let Dir = this.Direction
		
		Last.X = Pos.X
		Last.Y = Pos.Y

		this.Rotate(this.StateMachine[Grid[GridSize.X * Pos.Y + Pos.X] % this.StateMachine.length])
		Pos.X += Dir.X
		Pos.Y += Dir.Y
	
		if (Wrap) this.WrapPosition(GridSize)
	}
	
	UpdateCell(Grid, GridSize) {
		let Last = this.LastPosition
		let Pos = this.Position
	
		// Check if the last update has landed us in a invalid position
		// Positive out of bounds checks go first since a overflow will also trigger them
		if (Pos.X >= GridSize.X || Pos.Y >= GridSize.Y || Pos.X < 0 || Pos.Y < 0) return 0

		let CellIndex = GridSize.X * Last.Y + Last.X
		let CellValue = Grid[CellIndex]
	
		Grid[CellIndex] = (CellValue + 1) % this.StateMachine.length
	
		return 1
	}

	constructor(X, Y, DX, DY, StateMachine) {
		this.Position.X = X
		this.Position.Y = Y
		this.Direction.X = DX
		this.Direction.Y = DY
		this.StateMachine = StateMachine
	}
}