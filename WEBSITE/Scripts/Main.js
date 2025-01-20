

function ReadVec2Input(Item) {
	return { X: Number(Item.X.val()), Y: Number(Item.Y.val()) }

}

function SetVec2Input(Item, Value) {
	Item.X.val(Value.X)
	Item.Y.val(Value.Y)
}


async function Main() {
	let MainCanvas = $("#MainCanvas")

	let Stats = {
		FPS_IPS: $("#Stats_FPS_IPS"),
		Camera: $("#Stats_Camera"),

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

	function UnloadSiteLink() {
		let Serialized = JSON.stringify({
			Size: GridSize,
			Ants: SimulationObject.TemplateAnts.slice(0).map(AntObject => AntObject.ToObject()),
		})
		
		let Compressed = fflate.compressSync(fflate.strToU8(Serialized), { level: 9, mem: 12 })

		let Encoded = ""; Compressed.forEach((Value) => { Encoded += String.fromCharCode(Value) })
		
		window.history.replaceState(
			null,
			document.title,
			`${window.location.pathname}?v=${encodeURIComponent(btoa(Encoded))}`
		)
	}

	function LoadSiteLink() {
		let Encoded = new URL(window.location.href).searchParams.get("v")
		
		if (!Encoded)
			return

		let Compressed = new Uint8Array(atob(decodeURIComponent(Encoded)).split('').map(Value => Value.charCodeAt(0)))

		let Serialized = fflate.strFromU8(fflate.decompressSync(Compressed))

		let { Size, Ants } = JSON.parse(Serialized)

		SetVec2Input(GridConfig.Size, Size)

		for (let AntObject of Ants.map(Ant.FromObject)) {
			SimulationObject.TemplateAnts.push(AntObject)
		}
		
	}
	LoadSiteLink()

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

				Stats.Camera.html(`${Round(CameraPosition.X, 3)}, ${Round(CameraPosition.Y, 3)}, ${Round(CameraPosition.Z, 3)}`)
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
			*/
			Stats.Camera.html(`${Round(CameraPosition.X, 3)}, ${Round(CameraPosition.Y, 3)}, ${Round(CameraPosition.Z, 3)}`)
		})
	}

	// Setup grid configs
	{
		let {Size, Iterations} = GridConfig

		function UpdateGridSize() {
			GridData = SimulationObject.ResizeGrid(Number(Size.X.val()), Number(Size.Y.val()))

			Stats.Buffer.html(MeasureData(GridData.byteLength, 2))

			ReuploadTexture = true
			UnloadSiteLink()
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
		let Ant_Position = { X: $("#Ant_PositionX"), Y: $("#Ant_PositionY") }
		let Ant_Direction = { X: $("#Ant_DirectionX"), Y: $("#Ant_DirectionY") }
		let Ant_StepSize = $("#Ant_StepSize")
		
		let Ant_Rules = $("#Ant_Rules")
		let Ant_Rules_Mutate = $("#Ant_Rules_Mutate")
		let Ant_Rules_Randomize = $("#Ant_Rules_Randomize")

		let Ant_Wrap = $("#Ant_Wrap")
		let Ant_Enabled = $("#Ant_Enabled")
		
		let ElementToAnt = new Map()
		
		let Selected = null // Selected element
		let AntObject = null // Selected ant

		function UpdateOptions() {
			for (let Item of [...Object.values(Ant_Position), ...Object.values(Ant_Direction), Ant_StepSize, Ant_Rules, Ant_Wrap, Ant_Enabled]) {
				SetEnabled(Item, Selected)
			}

			UnloadSiteLink()
			SetEnabled(CloneAnt, Selected)

			if (!Selected) { return }
			AntObject = ElementToAnt.get(Selected)
			
			SetVec2Input(Ant_Direction, AntObject.Direction)
			SetVec2Input(Ant_Position, AntObject.Position)

			Ant_StepSize.val(AntObject.StepSize)
			Ant_Wrap.prop("checked", AntObject.Wrap)

			Ant_Enabled.prop("checked", SimulationObject.TemplateAnts.findIndex(Value => Value == AntObject) >= 0)

			Ant_Rules.val(StateMachineToString(AntObject.StateMachine))
		}

		function UpdateAnt() {
			if (!Selected) { return }
			
			AntObject.Direction = ReadVec2Input(Ant_Direction)
			AntObject.Position = ReadVec2Input(Ant_Position)

			AntObject.StepSize = Number(Ant_StepSize.val())

			AntObject.Wrap = Ant_Wrap.prop("checked")

			SimulationObject.Reset()
			UpdateOptions()
		}
		
		[
			...Object.values(Ant_Position),
			...Object.values(Ant_Direction),
			Ant_StepSize,
			Ant_Wrap
		].forEach(Item => Item.on("change", UpdateAnt))

		Ant_Enabled.on("change", () => {
			if (!Ant_Enabled.prop("checked")) {
				SimulationObject.RemoveAnt(AntObject)
			}
			else {
				SimulationObject.AddAnt(AntObject)
			}

			SimulationObject.Reset()
		})

		Ant_Rules.on("change", () => {
			if (!Selected) { return }

			let [StateMachine, Success] = ParseStateMachine(Ant_Rules.val())
			
			if (!Success) {
				Ant_Rules.css("background-color", "rgba(255, 0, 0, 0.2)")
				return
			}
			else {
				Ant_Rules.css("background-color", "rgba(0, 255, 0, 0.2)")
				AntObject.StateMachine = StateMachine
			}
			
			SimulationObject.Reset()
			UpdateOptions()
		})

		Ant_Rules_Mutate.on("click", () => {
			if (!Selected) { return }
			
			let States = Object.values(c_DirectionEnum)
			let StateMachine = AntObject.StateMachine
			
			StateMachine[Math.floor(Math.random() * StateMachine.length)] = States[Math.floor(Math.random() * States.length)]
			SimulationObject.Reset()
			UpdateOptions()
		})

		Ant_Rules_Randomize.on("click", () => {
			if (!Selected) { return }
			
			let States = Object.values(c_DirectionEnum)
			
			AntObject.StateMachine = Array.from({length: (Math.random() * 10) + 4}, () => States[Math.floor(Math.random() * States.length)])
			
			SimulationObject.Reset()
			UpdateOptions()
		})
		
		let AntList = $("#AntList")
		let CreateAnt = $("#CreateAnt")
		let RemoveAnt = $("#RemoveAnt")
		let CloneAnt = $("#CloneAnt")

		function AddAnt(NewAnt, Select) {
			let NewLabel = CreateElement("div", AntList, { class: "Item" + (Select ? " Selected" : "") }, `Ant ${AntList.children().length + 1}`)

			ElementToAnt.set(NewLabel, NewAnt)

			SetEnabled(RemoveAnt, true)

			if (Select) {
				SetEnabled(CloneAnt, true)
				AntObject = NewAnt
				Selected = NewLabel
			}

			SimulationObject.AddAnt(NewAnt)
			SimulationObject.Reset()
			UpdateOptions()
		}

		AntList.on("click", (EventObject) => {
			if (EventObject.target == EventObject.currentTarget) { return }
			if (Selected) { Selected.setAttribute("class", "Item") }

			if (EventObject.target == Selected) {
				Selected = null
				UpdateOptions()
				return
			}

			(Selected = EventObject.target).setAttribute("class", "Item Selected")
			UpdateOptions()
		})

		CreateAnt.on("click", () => {
			let States = Object.values(c_DirectionEnum)
			let NewAnt = new Ant(
				GridSize.X / 2, GridSize.Y / 2,
				0, -1,
				Array.from({length: (Math.random() * 10) + 4}, () => States[Math.floor(Math.random() * States.length)])
			)

			AddAnt(NewAnt, !Selected)
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
			UpdateOptions()
		})

		CloneAnt.on("click", () => {
			if (!Selected) { return }

			let NewAnt = new Ant(
				AntObject.Position.X, AntObject.Position.Y,
				AntObject.Direction.X, AntObject.Direction.Y,
				AntObject.StateMachine,
				AntObject.Wrap,
				AntObject.StepSize
			)

			AddAnt(NewAnt, false)
		})

		for (let AntObject of SimulationObject.TemplateAnts) {
			ElementToAnt.set(
				CreateElement("div", AntList, { class: "Item" }, `Ant ${AntList.children().length + 1}`),
				AntObject
			)
		}
		SimulationObject.Reset()

		SetEnabled(RemoveAnt, AntList.children.length > 0)
		UpdateOptions()
	}


	// Setup interface
	let Simulating = false
	{
		let StartStop = $("#Simulation_Pause")

		let Items = Object.values(GridConfig.Size)
		StartStop.on("click", () => {
			if (Simulating) {
				StartStop.html("Start")
				StartStop.css("background-color", "rgba(0, 255, 0, 0.2)")
				Simulating = false
	
				for (let Item of Items) SetEnabled(Item, true)
			}
			else {
				StartStop.html("Stop")
				StartStop.css("background-color", "rgba(255, 0, 0, 0.2)")
				Simulating = true
	
				for (let Item of Items) SetEnabled(Item, false)
			}
		})
	
		$("#Simulation_Reset").on("click", () => { ReuploadTexture = true; SimulationObject.Reset() })
		$("#Simulation_Step").on("click",  () => { ReuploadTexture = true; SimulationObject.Update(IPF) })
		$("#ResetCamera").on("click",      () => { CameraPosition = { X: 0, Y: 0, Z: 0 }; Stats.Camera.html("0, 0, 0") })
		$("#SaveImage").on("click",        () => { alert("Not yet implemented") })
		
		// Others
		$("#GitHubLink").on("click", () => { window.open("https://github.com/rafa-br34/LangtonsAnt", "_blank") })
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

		Stats.Bandwidth.html(`${Simulating ? MeasureData(GridData.byteLength * FPS) : "0B"}/s`)
		Stats.Iteration.html(SimulationObject.TotalIterations)
		Stats.LiveAnts.html(SimulationObject.Ants.length)
		Stats.FPS_IPS.html(`${Round(FPS, 2)}/${Round(FPS * IPF, 2)}`)
	}
	

	let Last = performance.now()
	function UpdateFrame(FrameTime) {
		let Delta = FrameTime - Last
		
		UpdateCounters(Delta)
		if (Simulating) SimulationObject.Update(IPF)
		
		DrawFrame(
			Shader,
			GL,
			ReuploadTexture || Simulating,
			GridSize, GridData,
			CameraPosition
		)
		ReuploadTexture = false

		Last = FrameTime
		return requestAnimationFrame(UpdateFrame)
	}
	requestAnimationFrame(UpdateFrame)
}

document.addEventListener("DOMContentLoaded", Main)
document.addEventListener("load", Main)