#version 300 es

precision highp float;

uniform highp usampler2D u_Grid;
uniform vec2 u_GridSize;
uniform vec3 u_Position;

in vec4 VertexColor;
out vec4 FragColor;

// From vertex shader
in vec4 u_VertexPosition;


#define XOR_SHIFT32(S) S ^= (S << 13); S ^= (S >> 17); S ^= (S << 5);

#define U8(V) uint(int(V) % 256)
#define I8(V) int((int(V) % 256) - 127)
#define FU8(V) float(U8(V)) / 255.0
#define FI8(V) float(I8(V)) / 255.0

vec4 GetColor(uint S) {
	// This won't create a very high quality random number but it doesn't really matter
	XOR_SHIFT32(S); S *= 0x9E3779B9u;

	return vec4(
		FU8((S >> 16) & 0xFFu),
		FU8((S >> 8) & 0xFFu),
		FU8((S >> 0) & 0xFFu),
		1.0
	);
}

/*
vec4 Get(float Grid, bool Antialias) {
	// Calculate the floating point difference between pixels
	vec2 PixelDelta = (u_VertexPosition.xy / u_Position.z) / u_GridSize;
	vec2 PixelCoverage = (PixelDelta * u_GridSize);

	#define N(X, Y) texelFetch(u_Grid, ivec2(u_VertexPosition.xy * u_GridSize) + ivec2(X, Y), 0).x
	#define C(X, Y) GetColor(N(X, Y))

	if (Grid > 0.0 && u_Position.z < -0.90 && ivec2(u_VertexPosition.xy * u_GridSize) != ivec2((u_VertexPosition.xy + (PixelDelta / Grid)) * u_GridSize))
		return vec4(1.0);
	else if (Antialias && (PixelCoverage.x > 1.0 || PixelCoverage.y > 1.0))
		return vec4(0.4);
	else
		return GetColor(texture(u_Grid, u_VertexPosition.xy).x);
}
*/

// @todo This shader can be improved by alot (adding antialiasing, better blending, grid when in high zoom, etc)
void main() {
	FragColor = GetColor(texture(u_Grid, u_VertexPosition.xy).x);;
	//FragColor = vec4(u_VertexPosition.x, u_VertexPosition.y, 0.0, 1.0);
	//FragColor = vec4(GetColor((uint(u_VertexPosition.x * 255.0) << 8) + uint(u_VertexPosition.y * 255.0)));
	//FragColor = vec4(float(texture(u_Grid, u_VertexPosition.xy).x) / 65535.0, 0.0, 0.0, 1.0);
}

