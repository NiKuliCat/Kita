#program   vertex    
#version 450 core

layout(location = 0) in vec3 PositionOS;



struct Attributes
{
	vec2 positionCS;
};

layout(location = 0) out Attributes Attri_Output;

void main()
{
	Attri_Output.positionCS = PositionOS.xy;

	gl_Position = vec4(PositionOS.xy,1.0, 1.0);
}


#program     fragment
#version 450 core


struct Attributes
{
	vec2 positionCS;
};

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

layout(location = 0) in Attributes Attri_Input;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int IDColor;


uniform samplerCube SkyboxTex;


void main()
{
	vec4 clip = vec4(Attri_Input.positionCS, 1.0, 1.0);   // 远平面
	vec4 view = Matrix_I_P * clip;
    view /= view.w;

	vec3 dirVS = normalize(view.xyz);
    vec3 dirWS = normalize((mat3(Matrix_I_V) * dirVS)); // 只要旋转

	FragColor = vec4(texture(SkyboxTex, dirWS).rgb,1.0);
	IDColor = -1;
}
