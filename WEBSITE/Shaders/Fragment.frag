#version 300 es

precision highp float;

// Uniforms
uniform highp usampler2D u_Grid;
uniform vec2 u_GridSize;
uniform vec3 u_Position;

// To output
out vec4 o_FragColor;

// From vertex shader
in vec4 s_VertexPosition;


#define XOR_SHIFT32(S) S ^= (S << 13); S ^= (S >> 17); S ^= (S << 5);

#define U8(V) uint(int(V) % 256)
#define I8(V) int((int(V) % 256) - 127)
#define FU8(V) float(U8(V)) / 255.0
#define FI8(V) float(I8(V)) / 255.0

vec3 GetColor(uint S) {
	// This won't create a very high quality random number but it doesn't really matter
	XOR_SHIFT32(S); S *= 0x9E3779B9u;

	return vec3(
		FU8((S >> 16) & 0xFFu),
		FU8((S >> 8) & 0xFFu),
		FU8((S >> 0) & 0xFFu)
	);
}

const float Grid = 10.0;
const bool Antialias = true;

// @todo This shader can be improved by alot (adding antialiasing, better blending, grid when in high zoom, etc)
void main() {
	o_FragColor = vec4(GetColor(texture(u_Grid, s_VertexPosition.xy).x), 1.0);
	//o_FragColor = vec4(s_VertexPosition.x, s_VertexPosition.y, 0.0, 1.0);
	//o_FragColor = vec4(GetColor((uint(s_VertexPosition.x * 255.0) << 8) + uint(s_VertexPosition.y * 255.0)));
	//o_FragColor = vec4(float(texture(u_Grid, s_VertexPosition.xy).x) / 65535.0, 0.0, 0.0, 1.0);
	
	/*
	// Calculate the floating point difference between pixels
	vec2 PixelDelta = (s_VertexPosition.xy / u_Position.z) / u_GridSize;
	vec2 PixelCoverage = (PixelDelta * u_GridSize);

	#define N(X, Y) texelFetch(u_Grid, ivec2(s_VertexPosition.xy * u_GridSize) + ivec2(X, Y), 0).x
	#define C(X, Y) GetColor(N(X, Y))

	if (Grid > 0.0 && u_Position.z < -0.90 && ivec2(s_VertexPosition.xy * u_GridSize) != ivec2((s_VertexPosition.xy + (PixelDelta / Grid)) * u_GridSize))
		o_FragColor = vec4(1.0);
	else
		o_FragColor = vec4(GetColor(texture(u_Grid, s_VertexPosition.xy).x), 1.0);
	*/
}

