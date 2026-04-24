#program   vertex    
#version 450 core
layout(location = 0) in vec3 PositionOS;

uniform mat4 Model;
uniform vec4 Color;

layout(std140, binding = 0) uniform CameraData
{
	mat4 Matrix_V;
	mat4 Matrix_P;
	mat4 Matrix_VP;
	mat4 Matrix_I_V;
	mat4 Matrix_I_P;
	mat4 Matrix_I_VP;
	vec4 CameraPosWS;
};

struct Attributes
{
	vec4 color;
};

layout(location = 0) out Attributes Attri_Output;

void main()
{
	Attri_Output.color = Color;
	gl_Position = Matrix_VP * Model *  vec4(PositionOS, 1.0);
}


#program     fragment
#version 450 core


struct Attributes
{
	vec4 color;
};

layout(location = 0) in Attributes Attri_Input;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int IDColor;

uniform int id;

void main()
{
	FragColor = vec4(Attri_Input.color.rgb,1.0);
	IDColor = id;
}
