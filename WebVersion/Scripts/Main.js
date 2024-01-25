function ResizeGrid(NewSize) {
	return new Uint16Array(NewSize.X * NewSize.Y).fill(0)
}

function SetEnabled(Item, Value) {
	if (!Value)
		Item.setAttribute("disabled", "true")
	else
		Item.removeAttribute("disabled")
}

async function Main() {
	let MainCanvas = document.getElementById("MainCanvas")
	
	let Stats = {
		FPS: document.getElementById("Stats_FPS"),
		IPS: document.getElementById("Stats_IPS"),
		X: document.getElementById("Stats_X"),
		Y: document.getElementById("Stats_Y"),
		Entropy: document.getElementById("Stats_Entropy"),
		Iteration: document.getElementById("Stats_Iteration"),
	}
	
	let Config = {
		GridSizeX: document.getElementById("GridSizeX"),
		GridSizeY: document.getElementById("GridSizeY")
	}
	
	let Interface = {
		SaveImage: document.getElementById("SaveImage"),
		ResetCamera: document.getElementById("ResetCamera"),
		ResetState: document.getElementById("ResetState"),
		StartStop: document.getElementById("StartStop")
	}

	let CameraPosition = { X: 0, Y: 0, Z: 0 }
	let GridSize = { X: 0, Y: 0 }
	let Grid
	let Run = false

	let Renderer = new GLRenderer()

	try {
		Renderer.Initialize(MainCanvas, { premultipliedAlpha: false, antialias: false })
	}
	catch (Exception) {
		alert("WebGL unsupported or unavailable\n(Failed to acquire WebGL context)")
		return
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

		function UpdateGridSize() {
			GridSize.X = Config.GridSizeX.value
			GridSize.Y = Config.GridSizeY.value

			DBG(`Resizing grid to ${GridSize.X}x${GridSize.Y} ${GridSize.X * GridSize.Y} items`)
			Grid = ResizeGrid(GridSize)
		}
		Config.GridSizeX.addEventListener("input", UpdateGridSize)
		Config.GridSizeY.addEventListener("input", UpdateGridSize)
		UpdateGridSize()

		MainCanvas.addEventListener("mousemove", (EventObject) => {
			if (!Run) return

			if (EventObject.buttons & 1) {
				CameraPosition.X += (EventObject.movementX / (MainCanvas.width / 2)) / (GridSize.X / GridSize.Y)
				CameraPosition.Y -= (EventObject.movementY / (MainCanvas.height / 2)) / (GridSize.Y / GridSize.X)

				Stats.X.innerHTML = `X: ${Math.round(CameraPosition.X * 1000) / 1000}`
				Stats.Y.innerHTML = `Y: ${Math.round(CameraPosition.Y * 1000) / 1000}`
			}
		})

		MainCanvas.addEventListener("wheel", (EventObject) => {
			if (!Run) return

			let ZoomFactor = (EventObject.deltaY / (MainCanvas.height / 2)) * 0.2

			CameraPosition.Z += (CameraPosition.Z + 1) * ZoomFactor
		})
	}

	let Positions = [
		-1, -1,
		-1,  1,
		 1, -1,

		-1,  1,
		 1, -1,
		 1,  1,
	]

	for (let i = 0; i < Positions.length; i++) { Positions[i] /= 2 }

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

	let StateMachine = CreateStateMachine("RLR".split('')) // LRRRRRLLR
	DBG(StateMachineToString(StateMachine))

	let Deltas = []
	let DeltaIndex = 0
	let BufferSize = 4
	let IterationsPerFrame = 100000
	let TotalIterations = 0
	let Ants = [ new Ant(GridSize.X / 2, GridSize.Y / 2, 0, -1, StateMachine) ]

	function UpdateCounters(Delta) {
		Deltas[DeltaIndex++] = (Delta / 1000)
		DeltaIndex %= BufferSize
		
		let Samples = Math.min(Deltas.length, BufferSize)
		let FPS = 0
		for (let i = 0; i < Samples; i++) {
			FPS += 1 / Deltas[i]
		}
		FPS /= Samples
		
		if (DeltaIndex == 0) {
			BufferSize = Math.round(Math.max(Math.min(FPS, 512), 32) * 0.5) // Automatically detect refresh rate and set buffer size to 0.5 seconds
		}

		Stats.FPS.innerHTML = `FPS: ${Math.round(Math.max(FPS, 0) * 100) / 100}`
		Stats.IPS.innerHTML = `IPS: ${Math.round(Math.max(FPS * IterationsPerFrame, 0) * 100) / 100}`

		Stats.Iteration.innerHTML = `Iteration: ${TotalIterations}`
	}
	
	function UpdateSimulation() {
		for (let i = 0; i < IterationsPerFrame; i++) {
			for (let AntObject of Ants) AntObject.UpdatePosition(Grid, GridSize, false)
			for (let AntObject of Ants) AntObject.UpdateCell(Grid, GridSize)
		}
		TotalIterations += IterationsPerFrame
	}


	function UpdateRender() {
		GL.clearColor(0, 0, 0, 0)
		GL.clear(GL.COLOR_BUFFER_BIT)

		GL.uniform3fv(Program.Variables["u_Position"], [CameraPosition.X * (1 + CameraPosition.Z), CameraPosition.Y * (1 + CameraPosition.Z), CameraPosition.Z])
		GL.uniform2fv(Program.Variables["u_GridSize"], [GridSize.X, GridSize.Y])

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

		GL.drawArrays(GL.TRIANGLES, 0, 6)
	}

	let Last = performance.now()
	function UpdateFrame(FrameTime) {
		let Delta = FrameTime - Last
		
		UpdateCounters(Delta)
		UpdateSimulation()
		UpdateRender()

		if (Run) requestAnimationFrame(UpdateFrame)
		Last = FrameTime
	}

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
			UpdateFrame(performance.now())

			for (let Item of Items) SetEnabled(Item, false)
		}
	})

	Interface.ResetCamera.addEventListener("click", () => { CameraPosition = { X: 0, Y: 0, Z: 0 } })

	Interface.ResetState.addEventListener("click", () => { Grid.fill(0); TotalIterations = 0 })
}

document.addEventListener("DOMContentLoaded", Main)
document.addEventListener("load", Main)