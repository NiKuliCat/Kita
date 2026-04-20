#program   vertex    
#version 450 core
layout(location = 0) in vec3 PositionOS;

uniform mat4 Model;
uniform vec4 Color;

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
	Attri_Output.color = Color;
	gl_Position = ViewProjection * Model *  vec4(PositionOS, 1.0);
}


#program     fragment
#version 450 core


struct Attributes
{
	vec4 color;
};

layout(location = 0) in Attributes Attri_Input;

layout(location = 0) out vec4 FragColor;

void main()
{
	FragColor = vec4(Attri_Input.color.rgb,1.0);
}
