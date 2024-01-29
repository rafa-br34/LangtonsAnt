function ResizeGrid(NewSize) {
	return new Uint16Array(NewSize.X * NewSize.Y).fill(0)
}

function SetEnabled(Item, Value) {
	if (!Value)
		Item.setAttribute("disabled", "true")
	else
		Item.removeAttribute("disabled")
}

function MakeElement(TagName, Parent, Attributes, InnerHTML) {
	let NewElement = document.createElement(TagName)

	if (typeof (Attributes) != "undefined") {
		for (let Key in Attributes) {
			NewElement.setAttribute(Key, Attributes[Key])
		}
	}

	NewElement.innerHTML = typeof (InnerHTML) != "undefined" ? InnerHTML : ""
	if (Parent) { Parent.appendChild(NewElement) }

	return NewElement
}

function ByteCountToMeasurement(Count) {
	const Measurements = [
		"B",
		"KB",
		"MB",
		"GB",
		"TB",
		"EB",
		"ZB",
		"YB",
		"RB",
		"QB",
		"UNK"
	]

	Iteration = 0
	while (Count >= 1024) {
		Count /= 1024
		Iteration++
	}


	let Type = ""
	if (Iteration > Measurements.length)
		Type = Measurements[ListSize - 1]
	else
		Type = Measurements[Iteration]

	return [Count, Type]
}

async function SetupRenderer(Canvas) {
	let Renderer = new GLRenderer()

	try {
		Renderer.Initialize(Canvas, { premultipliedAlpha: false, antialias: true })
	}
	catch (Exception) {
		alert("WebGL unsupported or unavailable\n(Failed to acquire WebGL context)")
		throw Error("WebGL unavailable.")
	}


	let Program = Renderer.AddProgram(
		await fetch("Shaders/Vertex.vert").then(R => R.text()),
		await fetch("Shaders/Fragment.frag").then(R => R.text())
	)
	.AddAttributes([
		"a_VertexPosition"
	])
	.AddUniforms([
		"u_Position",
		"u_GridSize"
	])
	.Use()

	let GL = Renderer.GL // @todo Implement remaining methods

	let Positions = [
		-1, -1,
		-1,  1,
		 1, -1,

		-1,  1,
		 1, -1,
		 1,  1,
	]


	let VertexPositionBuffer = GL.createBuffer()
	GL.bindBuffer(GL.ARRAY_BUFFER, VertexPositionBuffer)
	GL.bufferData(GL.ARRAY_BUFFER, new Float32Array(Positions), GL.STATIC_DRAW)

	let VertexArray = GL.createVertexArray()
	GL.bindVertexArray(VertexArray)
	GL.enableVertexAttribArray(Program.Variables["a_VertexPosition"])
	GL.vertexAttribPointer(Program.Variables["a_VertexPosition"], 2, GL.FLOAT, false, 0, 0)

	let Texture = GL.createTexture()
	GL.bindTexture(GL.TEXTURE_2D, Texture)
	GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MIN_FILTER, GL.NEAREST)
	GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MAG_FILTER, GL.NEAREST)
	GL.pixelStorei(GL.UNPACK_ALIGNMENT, 1)

	return {GL, Renderer, Program}
}

class Simulation {
	Wrap = false
	
	TotalIterations = 0
	TemplateAnts = []
	Ants = []

	Reset() {
		this.Ants = []
		for (let AntObject of this.TemplateAnts) {
			this.Ants.push(new Ant(AntObject.Position.X, AntObject.Position.Y, AntObject.Direction.X, AntObject.Direction.Y, AntObject.StateMachine))
		}
		
		this.TotalIterations = 0
	}

	Update(Grid, GridSize, Iterations=1, Wrap=this.Wrap) {
		let Ants = this.Ants

		for (let i = 0; i < Iterations; i++) {
			for (let AntObject of Ants) AntObject.UpdatePosition(Grid, GridSize, Wrap)
			for (let i = 0; i < Ants.length; i++) if (!Ants[i].UpdateCell(Grid, GridSize)) Ants.splice(i, 1)
		}

		this.TotalIterations += Iterations
	}
}


async function Main() {
	let MainCanvas = document.getElementById("MainCanvas")
	
	let Stats = {
		FPS: document.getElementById("Stats_FPS"),
		IPS: document.getElementById("Stats_IPS"),
		X: document.getElementById("Stats_X"),
		Y: document.getElementById("Stats_Y"),

		Ants: document.getElementById("Stats_Ants"),
		Iteration: document.getElementById("Stats_Iteration"),
		Bandwidth: document.getElementById("Stats_Bandwidth"),
		Buffer: document.getElementById("Stats_Buffer")
	}
	
	let Config = {
		GridSizeX: document.getElementById("GridSizeX"),
		GridSizeY: document.getElementById("GridSizeY"),
		Iterations: document.getElementById("Iterations"),
		Wrap: document.getElementById("Wrap")
	}
	
	let Interface = {
		SaveImage: document.getElementById("SaveImage"),
		ResetCamera: document.getElementById("ResetCamera"),
		ResetState: document.getElementById("ResetState"),
		StartStop: document.getElementById("StartStop")
	}

	let SimulationInterface = {
		AntList: document.getElementById("AntList"),
		AddAnt: document.getElementById("AddAnt"),
		RemoveAnt: document.getElementById("RemoveAnt"),
		
		AntPositionX: document.getElementById("AntPositionX"),
		AntPositionY: document.getElementById("AntPositionY"),
		AntDirectionX: document.getElementById("AntDirectionX"),
		AntDirectionY: document.getElementById("AntDirectionY"),
		AntRules: document.getElementById("AntRules")
	}

	
	let {GL, Renderer, Program} = await SetupRenderer(MainCanvas)
	
	let CameraPosition = { X: 0, Y: 0, Z: 0 }
	let GridSize = { X: 0, Y: 0 }
	let Grid

	// Setup Listeners
	{
		function UpdateCanvasSize() {
			let Size = MainCanvas.getClientRects()[0]
			
			let S = Size.width
			DBG(`Resizing canvas to ${S}x${S}`)
			MainCanvas.setAttribute("width", S)
			MainCanvas.setAttribute("height", S)
			
			Renderer.Viewport(0, 0, S, S)
		}
	
		window.addEventListener("resize", UpdateCanvasSize)
		UpdateCanvasSize()

		MainCanvas.addEventListener("mousemove", (EventObject) => {
			if (EventObject.buttons & 1) {
				let RX = GridSize.X / GridSize.Y
				let RY = GridSize.Y / GridSize.X

				let DX = (EventObject.movementX / (MainCanvas.width / 2))
				let DY = (EventObject.movementY / (MainCanvas.height / 2))

				if (RX > RY) {
					CameraPosition.X += DX / RX
					CameraPosition.Y -= DY
				}
				else {
					CameraPosition.X += DX
					CameraPosition.Y -= DY / RY
				}

				Stats.X.innerHTML = `X: ${Math.round(CameraPosition.X * 1000) / 1000}`
				Stats.Y.innerHTML = `Y: ${Math.round(CameraPosition.Y * 1000) / 1000}`
			}
		})

		MainCanvas.addEventListener("wheel", (EventObject) => {
			let ZoomFactor = (EventObject.deltaY / (MainCanvas.height / 2)) * 0.2

			CameraPosition.Z += (CameraPosition.Z + 1) * ZoomFactor
			DBG(ZoomFactor, CameraPosition.Z)
			
			// @todo Fix...
			/*
			// Calculate normalized device coordinates
			let X = (CameraPosition.X / 2) / -CameraPosition.Z
			let Y = (CameraPosition.Y / 2) / -CameraPosition.Z

			CameraPosition.X += X / 2
			CameraPosition.Y += Y / 2

			Stats.X.innerHTML = `X: ${Math.round(CameraPosition.X * 1000) / 1000}`
			Stats.Y.innerHTML = `Y: ${Math.round(CameraPosition.Y * 1000) / 1000}`
			*/
		})
	}

	let IterationsPerFrame = 1
	let Wrap = false
	// Setup configs
	{
		function UpdateGridSize() {
			GridSize.X = Config.GridSizeX.value
			GridSize.Y = Config.GridSizeY.value

			DBG(`Resizing grid to ${GridSize.X}x${GridSize.Y} ${GridSize.X * GridSize.Y} items`)
			Grid = ResizeGrid(GridSize)

			let [Count, Type] = ByteCountToMeasurement(Grid.byteLength)
			Stats.Buffer.innerHTML = `Buffer: ${Math.round(Count * 100) / 100}${Type}`
		}
		Config.GridSizeX.addEventListener("input", UpdateGridSize)
		Config.GridSizeY.addEventListener("input", UpdateGridSize)
		UpdateGridSize()

		function UpdateIterations() {
			DBG(`New IPF value is ${IterationsPerFrame = Number(Config.Iterations.value)}`)
		}
		Config.Iterations.addEventListener("input", UpdateIterations)
		UpdateIterations()

		function UpdateWrap() {
			DBG(`New Wrap value is ${Wrap = Config.Wrap.checked}`)
		}
		Config.Wrap.addEventListener("change", UpdateWrap)
		UpdateWrap()
	}

	let SimulationObject = new Simulation()	
	// Setup simulation interface
	{
		let {AntList, AddAnt, RemoveAnt, AntPositionX, AntPositionY, AntDirectionX, AntDirectionY, AntRules} = SimulationInterface
		let AntOptions = [AntPositionX, AntPositionY, AntDirectionX, AntDirectionY, AntRules]
		
		let RemoveByIndex = (Table, Item) => { let Index = 0; if ((Index = Table.findIndex(Value => Value == Item)) >= 0) Table.splice(Index, 1) }

		let SelectedAnt = null
		function UpdateOptions() {
			for (let Item of AntOptions) SetEnabled(Item, SelectedAnt)

			if (!SelectedAnt) { return }
			let AntObject = SelectedAnt.AntObject

			AntPositionX.value = AntObject.Position.X
			AntPositionY.value = AntObject.Position.Y

			AntDirectionX.value = AntObject.Direction.X
			AntDirectionY.value = AntObject.Direction.Y

			AntRules.value = StateMachineToString(AntObject.StateMachine)
		}
		
		AntList.addEventListener("click", (EventObject) => {
			if (EventObject.target == AntList) { return }
			if (SelectedAnt) { SelectedAnt.setAttribute("style", "") }

			(SelectedAnt = EventObject.target).setAttribute("style", "background: rgba(0,0,0,0.1);")
			UpdateOptions()
		})

		AddAnt.addEventListener("click", () => {
			let NewAnt = new Ant(GridSize.X / 2, GridSize.Y / 2, 0, -1, CreateStateMachine("RL".split('')))
			let NewLabel = MakeElement("div", AntList, { class: "ListItem" }, `Ant ${performance.now()}`)

			NewLabel.AntObject = NewAnt
			SimulationObject.TemplateAnts.push(NewAnt)

			SetEnabled(RemoveAnt, true)
			Grid.fill(0); SimulationObject.Reset()
		})
		
		RemoveAnt.addEventListener("click", () => {
			SetEnabled(RemoveAnt, AntList.children.length - 1 > 0)

			if (!SelectedAnt && !AntList.children.length) { return }

			SelectedAnt = SelectedAnt || AntList.children[0]

			let AntObject = SelectedAnt.AntObject

			RemoveByIndex(SimulationObject.TemplateAnts, AntObject)

			SelectedAnt.remove()
			SelectedAnt = null
			Grid.fill(0); SimulationObject.Reset()
		})

		function UpdateAnt() {
			if (!SelectedAnt) { return }
			let AntObject = SelectedAnt.AntObject

			AntObject.Position.X = Number(AntPositionX.value)
			AntObject.Position.Y = Number(AntPositionY.value)
			
			AntObject.Direction.X = Number(AntDirectionX.value)
			AntObject.Direction.Y = Number(AntDirectionY.value)

			Grid.fill(0); SimulationObject.Reset()
		}
		AntPositionX.addEventListener("change", UpdateAnt)
		AntPositionY.addEventListener("change", UpdateAnt)
		AntDirectionX.addEventListener("change", UpdateAnt)
		AntDirectionY.addEventListener("change", UpdateAnt)

		AntRules.addEventListener("change", () => {
			if (!SelectedAnt) { return }
			let AntObject = SelectedAnt.AntObject

			let RuleString = AntRules.value
			let RuleSet = []

			while (RuleString.length) {
				let Result = null
				if (typeof(Result = c_DirectionEnum[RuleString.substring(0, 4)]) == "number") { RuleString = RuleString.substring(4) }
				else if (typeof(Result = c_DirectionEnum[RuleString.substring(0, 3)]) == "number") { RuleString = RuleString.substring(3) }
				else if (typeof(Result = c_DirectionEnum[RuleString.substring(0, 1)]) == "number") { RuleString = RuleString.slice(1) }
				
				if (Result != null) {
					RuleSet.push(Result)
				}
				else {
					AntRules.style.backgroundColor = "rgba(255,0,0,0.5)"
					return
				}
			}
			AntRules.style.backgroundColor = "rgba(0,255,0,0.5)"
			AntObject.StateMachine = RuleSet
			
			Grid.fill(0); SimulationObject.Reset()
			UpdateOptions()
		})

		UpdateOptions()
		SetEnabled(RemoveAnt, AntList.children.length)
	}


	// Setup interface
	let Run = false
	{
		let Items = [ Config.GridSizeX, Config.GridSizeY ]
		Interface.StartStop.addEventListener("click", () => {
			if (Run) {
				Interface.StartStop.innerHTML = "Start"
				Run = false
	
				for (let Item of Items) SetEnabled(Item, true)
			}
			else {
				Interface.StartStop.innerHTML = "Stop"
				Run = true
	
				for (let Item of Items) SetEnabled(Item, false)
			}
		})
	
		Interface.ResetCamera.addEventListener("click", () => { CameraPosition = { X: 0, Y: 0, Z: 0 } })
	
		Interface.ResetState.addEventListener("click", () => { Grid.fill(0); SimulationObject.Reset() })
	}

	//SimulationObject.TemplateAnts.push(new Ant(500, 800, 0, -1, CreateStateMachine("RL".split(''))))

	let Deltas = []
	let DeltaIndex = 0
	let BufferSize = 4
	

	function UpdateCounters(Delta) {
		Deltas[DeltaIndex++] = (Delta / 1000)
		DeltaIndex %= BufferSize
		
		let Samples = Math.min(Deltas.length, BufferSize)
		let FPS = 0
		for (let i = 0; i < Samples; i++) {
			FPS += 1 / (Deltas[i] || Number.MAX_SAFE_INTEGER)
		}
		FPS /= Samples

		if (DeltaIndex == 0) {
			BufferSize = Math.round(Math.max(Math.min(FPS, 512), 32) * 0.5) // Automatically detect refresh rate and set buffer size to 0.5 seconds
		}

		Stats.FPS.innerHTML = `FPS: ${Math.round(Math.max(FPS, 0) * 100) / 100}`
		Stats.IPS.innerHTML = `IPS: ${Math.round(Math.max(FPS * IterationsPerFrame, 0) * 100) / 100}`

		let [Count, Type] = ByteCountToMeasurement(Grid.byteLength * FPS)
		Stats.Iteration.innerHTML = `Iteration: ${SimulationObject.TotalIterations}`
		Stats.Bandwidth.innerHTML = `Bandwidth: ${Math.round(Math.max(Count, 0) * 100) / 100}${Type}/s`
		Stats.Ants.innerHTML = `Ants: ${SimulationObject.TemplateAnts.length}`
	}
	

	let LastSize = GridSize.X * GridSize.Y
	function UpdateRender() {
		GL.clearColor(0, 0, 0, 0)
		GL.clear(GL.COLOR_BUFFER_BIT)

		GL.uniform3fv(Program.Variables["u_Position"], [CameraPosition.X * (1 + CameraPosition.Z), CameraPosition.Y * (1 + CameraPosition.Z), CameraPosition.Z])
		GL.uniform2fv(Program.Variables["u_GridSize"], [GridSize.X, GridSize.Y])

		// There's no point in uploading a new buffer if the simulation is not evolving and the buffer size didn't change
		if (Run || LastSize != GridSize.X * GridSize.Y) {
			GL.texImage2D(
				GL.TEXTURE_2D,
				0,
				GL.R16UI,
				GridSize.X,
				GridSize.Y,
				0,
				GL.RED_INTEGER,
				GL.UNSIGNED_SHORT,
				Grid,
			)
			LastSize = GridSize.X * GridSize.Y
		}

		GL.drawArrays(GL.TRIANGLES, 0, 6)
	}

	let Last = performance.now()
	function UpdateFrame(FrameTime) {
		let Delta = FrameTime - Last
		
		UpdateCounters(Delta)
		if (Run) SimulationObject.Update(Grid, GridSize, IterationsPerFrame, Wrap)
		UpdateRender()

		Last = FrameTime
		return requestAnimationFrame(UpdateFrame)
	}
	UpdateFrame(performance.now())
}

document.addEventListener("DOMContentLoaded", Main)
document.addEventListener("load", Main)