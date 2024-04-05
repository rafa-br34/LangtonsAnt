#version 300 es

precision highp float;

// Uniforms
uniform highp usampler2D u_Grid;
uniform vec2 u_GridSize;
uniform vec3 u_Position;

// To output
out vec4 o_FragColor;

// From vertex shader
in vec2 s_VertexPosition;


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

// @todo This shader can be improved
void main() {
	//o_FragColor = vec4(vec2(1.0) - s_VertexPosition.xy, 0.0, 1.0);
	o_FragColor = vec4(GetColor(texture(u_Grid, s_VertexPosition.xy).x), 1.0);
}

