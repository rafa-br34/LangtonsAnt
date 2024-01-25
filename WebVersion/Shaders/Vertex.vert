#version 300 es
 

uniform vec3 u_Position;
uniform vec2 u_GridSize;

in vec4 a_VertexPosition;

// To fragment shader
out vec4 u_VertexPosition;

void main() {
	gl_Position = (a_VertexPosition + vec4(u_Position.x, u_Position.y, 0.0, u_Position.z)) * vec4(u_GridSize.x / u_GridSize.y, u_GridSize.y / u_GridSize.x, 1.0, 1.0);

	u_VertexPosition = a_VertexPosition + 0.5;
}