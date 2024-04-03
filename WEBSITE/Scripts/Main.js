function CreateContext(Canvas, Vertex, Fragment) {
	let Renderer = new Context()

	try {
		Renderer.Initialize(Canvas, { premultipliedAlpha: false, antialias: true })
	}
	catch (Exception) {
		alert("WebGL unsupported or unavailable\n(Failed to acquire WebGL context)")
		throw Error("WebGL unavailable.")
	}


	let Shader = Renderer.AddProgram(
		Vertex,
		Fragment
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

	// Setup VBO
	let VBO = GL.createBuffer()
	GL.bindBuffer(GL.ARRAY_BUFFER, VBO)
	GL.bufferData(GL.ARRAY_BUFFER, new Float32Array(Positions), GL.STATIC_DRAW)

	// Setup VAO
	let VAO = GL.createVertexArray()
	GL.bindVertexArray(VAO)
	GL.enableVertexAttribArray(Shader.Variables["a_VertexPosition"])
	GL.vertexAttribPointer(Shader.Variables["a_VertexPosition"], 2, GL.FLOAT, false, 0, 0)

	let Texture = GL.createTexture()
	GL.bindTexture(GL.TEXTURE_2D, Texture)
	GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MIN_FILTER, GL.NEAREST)
	GL.texParameteri(GL.TEXTURE_2D, GL.TEXTURE_MAG_FILTER, GL.NEAREST)
	GL.pixelStorei(GL.UNPACK_ALIGNMENT, 1)

	return {GL, Renderer, Shader}
}

function Draw(Shader, GL, UploadTexture, Size, Data, CameraPosition) {
	GL.clearColor(0, 0, 0, 0)
	GL.clear(GL.COLOR_BUFFER_BIT)

	GL.uniform3fv(Shader.Variables["u_Position"], [CameraPosition.X * (1 + CameraPosition.Z), CameraPosition.Y * (1 + CameraPosition.Z), CameraPosition.Z])
	GL.uniform2fv(Shader.Variables["u_GridSize"], [Size.X, Size.Y])

	if (UploadTexture) {
		GL.texImage2D(
			GL.TEXTURE_2D,
			0,
			GL.R16UI,
			Size.X,
			Size.Y,
			0,
			GL.RED_INTEGER,
			GL.UNSIGNED_SHORT,
			Data,
		)
	}

	GL.drawArrays(GL.TRIANGLES, 0, 6)
}

async function Main() {
	let MainCanvas = $("#MainCanvas")

	let Stats = {
		FPS: $("#Stats_FPS"),
		IPS: $("#Stats_IPS"),
		X: $("#Stats_X"),
		Y: $("#Stats_Y"),

		LiveAnts: $("#Stats_LiveAnts"),
		Iteration: $("#Stats_Iteration"),
		Bandwidth: $("#Stats_Bandwidth"),
		Buffer: $("#Stats_Buffer")
	}

	let GridConfig = {
		Size: { X: $("#GridSizeX"), Y: $("#GridSizeY") },
		Iterations: $("#Iterations")
	}

	let SimulationObject = new SimulationState()
	let Shaders = { Vertex: null, Fragment: null }

	await Promise.all([
		fetch("Shaders/Vertex.vert").then(Result => Result.text()),
		fetch("Shaders/Fragment.frag").then(Result => Result.text())
	])
	.then(([Vertex, Fragment]) => {
		Shaders = { Vertex: Vertex, Fragment: Fragment }
	})
	.catch(console.error)
	
	let {GL, Renderer, Shader} = CreateContext(MainCanvas.get(0), Shaders.Vertex, Shaders.Fragment)

	let CameraPosition = { X: 0, Y: 0, Z: 0 }
	let GridSize = SimulationObject.GridSize
	let GridData = null
	let IPF = 1

	let ReuploadTexture = false

	// Setup canvas listeners
	{
		function UpdateCanvasSize() {
			let Width = MainCanvas.width()
			
			DBG(`Resizing canvas to ${Width}x${Width}`)
			MainCanvas.attr("width", Width)
			MainCanvas.attr("height", Width)
			
			Renderer.Viewport(Width, Width)
		}
	
		window.addEventListener("resize", UpdateCanvasSize)
		UpdateCanvasSize()

		MainCanvas.on("mousemove", (EventObject) => {
			if (EventObject.buttons & 1) {
				let RX = GridSize.X / GridSize.Y
				let RY = GridSize.Y / GridSize.X

				let DX = (EventObject.originalEvent.movementX / (MainCanvas.width() / 2))
				let DY = (EventObject.originalEvent.movementY / (MainCanvas.height() / 2))

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

		MainCanvas.on("wheel", (EventObject) => {
			let ZoomFactor = (EventObject.originalEvent.deltaY / (MainCanvas.height() / 2)) * 0.2
			CameraPosition.Z += (CameraPosition.Z + 1) * ZoomFactor
			
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

	// Setup grid configs
	{
		let {Size, Iterations} = GridConfig

		function UpdateGridSize() {
			GridData = SimulationObject.ResizeGrid(Number(Size.X.val()), Number(Size.Y.val()))

			Stats.Buffer.html(`Buffer: ${MeasureData(GridData.byteLength, 2)}`)

			ReuploadTexture = true
		}
		Size.X.on("input", UpdateGridSize)
		Size.Y.on("input", UpdateGridSize)
		UpdateGridSize()

		function UpdateIterations() {
			IPF = Math.round(Number(Iterations.val()))
		}
		Iterations.on("input", UpdateIterations)
		UpdateIterations()
	}

	// Setup ant interface
	{
		let Position = { X: $("#AntPositionX"), Y: $("#AntPositionY") }
		let Direction = { X: $("#AntDirectionX"), Y: $("#AntDirectionY") }
		let StepSize = $("#AntStepSize")
		let Rules = $("#AntRules")
		let Wrap = $("#AntWrap")
		
		let ElementToAnt = new Map()
		
		let Selected = null // Selected element
		let AntObject = null // Selected ant

		function UpdateOptions() {
			for (let Item of [...Object.values(Position), ...Object.values(Direction), Rules]) {
				SetEnabled(Item, Selected)
			}

			if (!Selected) { return }
			AntObject = ElementToAnt.get(Selected)

			Position.X.val(AntObject.Position.X)
			Position.Y.val(AntObject.Position.Y)

			Direction.X.val(AntObject.Direction.X)
			Direction.Y.val(AntObject.Direction.Y)

			StepSize.val(AntObject.StepSize)
			Wrap.prop("checked", AntObject.Wrap)

			Rules.val(StateMachineToString(AntObject.StateMachine))
		}
		
		let AntList = $("#AntList"), CreateAnt = $("#CreateAnt"), RemoveAnt = $("#RemoveAnt")

		AntList.on("click", (EventObject) => {
			if (EventObject.target == EventObject.currentTarget) { return }
			if (Selected) { Selected.setAttribute("style", "") }

			(Selected = EventObject.target).setAttribute("style", "background: rgba(0,0,0,0.1);")
			UpdateOptions()
		})

		CreateAnt.on("click", () => {
			let States = Object.values(c_DirectionEnum)
			let NewAnt = new Ant(
				GridSize.X / 2, GridSize.Y / 2,
				0, -1,
				Array.from({length: (Math.random() * 22) + 6}, () => States[Math.floor(Math.random() * States.length)])
			)

			let NewLabel = CreateElement("div", AntList, { class: "ListItem" }, `Ant ${AntList.children().length + 1}`)

			ElementToAnt.set(NewLabel, NewAnt)
			SimulationObject.AddAnt(NewAnt)

			SetEnabled(RemoveAnt, true)
			SimulationObject.Reset()
		})
		
		RemoveAnt.on("click", () => {
			let Children = AntList.children()

			SetEnabled(RemoveAnt, Children.length - 1 > 0)
			if (!Selected && !Children.length) { return }

			Selected = Selected || Children[0]
			
			AntObject = ElementToAnt.get(Selected)
			
			Selected.remove()
			Selected = null
			
			SimulationObject.RemoveAnt(AntObject)
			SimulationObject.Reset()
		})

		function UpdateAnt() {
			if (!Selected) { return }
			AntObject.Position.X = Number(Position.X.val())
			AntObject.Position.Y = Number(Position.Y.val())
			
			AntObject.Direction.X = Number(Direction.X.val())
			AntObject.Direction.Y = Number(Direction.Y.val())

			AntObject.StepSize = Number(StepSize.val())

			AntObject.Wrap = Wrap.prop("checked")

			SimulationObject.Reset()
		}
		[...Object.values(Position), ...Object.values(Direction), StepSize, Wrap].forEach(Item => Item.on("change", UpdateAnt))


		Rules.on("change", () => {
			if (!Selected) { return }

			let [StateMachine, Success] = ParseStateMachine(Rules.val())
			
			if (!Success) {
				Rules.css("background-color", "rgba(255, 0, 0, 0.2)")
				return
			}
			else {
				Rules.css("background-color", "rgba(0, 255, 0, 0.2)")
				AntObject.StateMachine = StateMachine
			}
			
			SimulationObject.Reset()
			UpdateOptions()
		})

		UpdateOptions()
		SetEnabled(RemoveAnt, AntList.children.length > 0)
	}


	// Setup interface
	let Simulating = false
	{
		let StartStop = $("#StartStop")

		let Items = Object.values(GridConfig.Size)
		StartStop.on("click", () => {
			if (Simulating) {
				StartStop.html("Start")
				Simulating = false
	
				for (let Item of Items) SetEnabled(Item, true)
			}
			else {
				StartStop.html("Stop")
				Simulating = true
	
				for (let Item of Items) SetEnabled(Item, false)
			}
		})
	
		$("#ResetCamera").on("click", () => { CameraPosition = { X: 0, Y: 0, Z: 0 } })
		$("#ResetState").on("click",  () => { ReuploadTexture = true; SimulationObject.Reset() })
		$("#SaveImage").on("click",   () => { alert("Not yet implemented") })
	}

	let FrameTimer = new FrameTimes()
	
	FrameTimer.TimeScale = 1
	function UpdateCounters(Delta) {
		FrameTimer.AddSample(Delta)
		FrameTimer.Update()
		
		let FPS = 1000 / FrameTimer.Average
		
		if (FrameTimer.SampleIndex == 0) {
			FrameTimer.SampleCount = Math.round(Math.max(Math.min(FPS, 512), 32) * 0.5) // Automatically detect refresh rate and set buffer size to 0.5 seconds
		}

		Stats.FPS.html(`FPS: ${Round(FPS, 2)}`)
		Stats.IPS.html(`IPS: ${Round(FPS * IPF, 2)}`)

		Stats.Iteration.html(`Iteration: ${SimulationObject.TotalIterations}`)
		Stats.Bandwidth.html(`Bandwidth: ${MeasureData(GridData.byteLength * FPS)}/s`)
		Stats.LiveAnts.html(`Live Ants: ${SimulationObject.Ants.length}`)
	}
	

	let Last = performance.now()
	function UpdateFrame(FrameTime) {
		let Delta = FrameTime - Last
		
		UpdateCounters(Delta)
		if (Simulating) SimulationObject.Update(IPF)
		
		Draw(Shader, GL, ReuploadTexture || Simulating, GridSize, GridData, CameraPosition); ReuploadTexture = false

		Last = FrameTime
		return requestAnimationFrame(UpdateFrame)
	}
	requestAnimationFrame(UpdateFrame)
}

document.addEventListener("DOMContentLoaded", Main)
document.addEventListener("load", Main)