function ExtendContext(Context, Extension) {
	return (Context.getExtension(Extension) ? true : alert(`Extension ${Extension} not supported or unavailable`)) || false
}

function InitializeContext(Canvas, Options) {

}

function CreateShader(GL, Source, Type) {
	let Shader = GL.createShader(Type)

	GL.shaderSource(Shader, Source)
	GL.compileShader(Shader)
	if (!GL.getShaderParameter(Shader, GL.COMPILE_STATUS)) {
		console.error(GL.getShaderInfoLog(Shader))
		throw Error(`Shader compilation failed`)
	}
	return Shader
}

class GLProgram {
	Variables = {}
	Program = null
	Parent = null

	Use() {
		if (!this.Program) return
		this.Parent.GL.useProgram(this.Program)

		return this
	}

	Delete() {
		if (this.Program) {
			this.Parent.GL.deleteProgram(this.Program)
			this.Program = null
		}
		
		return this
	}

	LoadShaders(VertexShader, FragmentShader) {
		this.Delete()
		
		let GL = this.Parent.GL

		let Program = this.Program = GL.createProgram()
	
		GL.attachShader(Program, CreateShader(GL, VertexShader, GL.VERTEX_SHADER))
		GL.attachShader(Program, CreateShader(GL, FragmentShader, GL.FRAGMENT_SHADER))
		GL.linkProgram(Program)
		
		if (!GL.getProgramParameter(Program, GL.LINK_STATUS)) throw Error(`Linking failed:\n${GL.getProgramInfoLog(Program)}`)

		return this
	}

	_AddVariable(Name, FunctionName) {
		if (this.Variables[Name]) throw Error(`Duplicate variable "${Name}"`)

		this.Variables[Name] = this.Parent.GL[FunctionName](this.Program, Name)
	}

	_AddVariables(Names, FunctionName) {
		if (typeof(Names) == "string")
			this._AddVariable(Names, FunctionName)
		else
			for (let Name of Names) this._AddVariable(Name, FunctionName)

		return this
	}
	
	AddAttributes(Names) { return this._AddVariables(Names, "getAttribLocation") }
	AddUniforms(Names) { return this._AddVariables(Names, "getUniformLocation") }

	constructor(Parent, VertexShaderOrProgram, FragmentShader) {
		this.Parent = Parent

		if (VertexShaderOrProgram && FragmentShader)
			this.LoadShaders(VertexShaderOrProgram, FragmentShader)
		else
			this.Program = VertexShaderOrProgram
	}
}

class GLRenderer {
	GL = null

	Programs = []


	Initialize(Canvas, Options) {
		this.GL = Canvas.getContext("webgl2", Options)

		if (this.GL == null) throw Error("WebGL unsupported or unavailable")
	
		return this
	}


	AddProgram(VertexShader, FragmentShader) {
		let NewProgram = new GLProgram(this, VertexShader, FragmentShader)
	
		this.Programs.push(NewProgram)
		return NewProgram
	}

	UseProgram(Program) {
		if (Program.constructor == GLProgram)
			Program.Use(this)
		else if (Program.constructor == WebGLProgram)
			this.GL.useProgram(Program)
		else
			throw Error(`Class "${Program.name}" is not valid as a program`)
	}


	Viewport(SX, SY, EX, EY) {
		this.GL.viewport(SX, SY, EX, EY)
	}
}