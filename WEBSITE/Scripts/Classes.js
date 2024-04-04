class SimulationState {
	GridSize = { X: 0, Y: 0 }
	Grid = null
	
	TotalIterations = 0
	TemplateAnts = []
	Ants = []

	Reset() {
		this.Ants = []
		for (let AntObject of this.TemplateAnts) {
			this.Ants.push(new Ant(AntObject.Position.X, AntObject.Position.Y, AntObject.Direction.X, AntObject.Direction.Y, AntObject.StateMachine, AntObject.Wrap, AntObject.StepSize))
		}
		
		this.TotalIterations = 0
		this.Grid.fill(0)
	}

	AddAnt(AntObject) {
		this.TemplateAnts.push(AntObject)
	}

	RemoveAnt(AntObject) {
		let List = this.TemplateAnts
		
		let Index = List.findIndex(Value => Value == AntObject)
		if (Index >= 0) {
			List.splice(Index, 1)
		}
	}

	Update(Iterations=1) {
		let GridSize = this.GridSize
		let Grid = this.Grid
		let Ants = this.Ants
		
		if (Ants.length == 1) { // Use single step update (faster)
			let AntObject = Ants[0]
			for (let i = 0; i < Iterations; i++) {
				if (!AntObject.Update(Grid, GridSize)) Ants.splice(0, 1)
			}
		}
		else { // Use double step update (slower)
			for (let i = 0; i < Iterations; i++) {
				for (let AntObject of Ants) AntObject.UpdatePosition(Grid, GridSize)
				for (let i = 0; i < Ants.length; i++) if (!Ants[i].UpdateCell(Grid, GridSize)) Ants.splice(i, 1)
			}
		}
		
		this.TotalIterations += Iterations
	}

	ResizeGrid(X, Y) {
		let GridSize = this.GridSize
		
		GridSize.X = X
		GridSize.Y = Y

		return this.Grid = new Uint16Array(GridSize.X * GridSize.Y).fill(0)
	}
}

class FrameTimes {
	SampleCount = 1; SampleIndex = 0
	Min = 0; Max = 0; Average = 0

	Samples = []
	TimeScale = 1000 // How much each sample should be multiplied to represent milliseconds

	Update() {
		let Average = 0
		let Min = this.Samples[0]
		let Max = 0

		let SampleCount = Math.min(this.Samples.length, this.SampleCount)

		for (let i = 0; i < SampleCount; i++) {
			let Sample = this.Samples[i]
			Min = Math.min(Min, Sample)
			Max = Math.max(Max, Sample)
			Average += (Sample - Average) / (i + 1)
		}

		this.Average = Average
		this.Min = Min
		this.Max = Max
	}

	AddSample(Sample) {
		this.Samples[this.SampleIndex] = Sample * this.TimeScale;
		this.SampleIndex = (this.SampleIndex + 1) % this.SampleCount;
	}
}