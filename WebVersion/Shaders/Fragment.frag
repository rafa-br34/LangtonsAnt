#version 300 es

precision highp float;

uniform highp usampler2D u_Grid;
uniform vec2 u_GridSize;


in vec4 VertexColor;
out vec4 FragColor;

// From vertex shader
in vec4 u_VertexPosition;


#define XOR_SHIFT32(S) S ^= (S << 13); S ^= (S >> 17); S ^= (S << 5);

#define U8(V) uint(int(V) % 256)
#define I8(V) int((int(V) % 256) - 127)
#define FU8(V) float(U8(V)) / 255.0
#define FI8(V) float(I8(V)) / 255.0

// @todo This shader can be improved by alot (adding antialiasing, better blending, grid when in high zoom, etc)
void main() {
///*
	uint S = texture(u_Grid, u_VertexPosition.xy).x;
	XOR_SHIFT32(S);
	S *= 0x9E3779B9u;
	//S *= 0xFFFFFFu;

	FragColor = vec4(
		FU8((S >> 16) & 0xFFu),
		FU8((S >> 8) & 0xFFu),
		FU8((S >> 0) & 0xFFu),
		1.0
	);
//*/
	//FragColor = vec4(u_VertexPosition.x, u_VertexPosition.y, 0.0, 1.0);
	//FragColor = vec4(0.0, 0.0, 0.0, 1.0);
	//FragColor = vec4(float(texture(u_Grid, u_VertexPosition.xy).x) / 65535.0, 0.0, 0.0, 1.0);
}

