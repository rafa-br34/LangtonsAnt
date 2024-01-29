#version 300 es
 

uniform vec3 u_Position;
uniform vec2 u_GridSize;

in vec4 a_VertexPosition;

// To fragment shader
out vec4 u_VertexPosition;

void main() {
	vec4 Position = (a_VertexPosition + vec4(u_Position.x, u_Position.y, 0.0, u_Position.z));

	float RX = u_GridSize.x / u_GridSize.y;
	float RY = u_GridSize.y / u_GridSize.x;
	
	if (RX > RY)
		gl_Position = Position * vec4(RX, 1.0, 1.0, 1.0);
	else
		gl_Position = Position * vec4(1.0, RY, 1.0, 1.0);

	u_VertexPosition = (a_VertexPosition + 1.0) / 2.0;
}