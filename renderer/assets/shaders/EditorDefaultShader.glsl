#program   vertex    
#version 450 core
layout(location = 0) in vec3 PositionOS;
layout(location = 1) in vec4 VertexColor;

layout(std140, binding = 0) uniform Camera
{
	mat4 ViewProjection;
};

struct Attributes
{
	vec4 color;
};

layout(location = 0) out Attributes Attri_Output;

void main()
{
	Attri_Output.color = VertexColor;
	gl_Position = vec4(PositionOS, 1.0);
}


#program     fragment
#version 450 core
layout(location = 0) out vec4 FragColor;

struct Attributes
{
	vec4 color;
};
layout(location = 0) in Attributes Attri_Input;

void main()
{
	FragColor = Attri_Input.color;
}