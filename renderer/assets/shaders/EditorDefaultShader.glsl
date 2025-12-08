#program   vertex    
#version 450 core
layout(location = 0) in vec3 PositionOS;
layout(location = 1) in vec4 VertexColor;
layout(location = 2) in vec2 Texcoord;

layout(std140, binding = 0) uniform Camera
{
	mat4 ViewProjection;
};

struct Attributes
{
	vec4 color;
	vec2 uv;
};

layout(location = 0) out Attributes Attri_Output;

void main()
{
	Attri_Output.color = VertexColor;
	Attri_Output.uv =  Texcoord;
	gl_Position = ViewProjection * vec4(PositionOS, 1.0);
}


#program     fragment
#version 450 core


struct Attributes
{
	vec4 color;
	vec2 uv;
};
layout(location = 0) in Attributes Attri_Input;

layout(location = 0) out vec4 FragColor;


uniform sampler2D MainTex;
void main()
{
	//FragColor = vec4(Attri_Input.uv,0.0,1.0);
	FragColor = vec4(texture(MainTex, Attri_Input.uv).rgb,1.0);
}