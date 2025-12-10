#program   vertex    
#version 450 core
layout(location = 0) in vec3 PositionOS;
layout(location = 1) in vec4 VertexColor;
layout(location = 2) in vec2 Texcoord;
layout(location = 3) in vec3 Normal;

uniform mat4 Model;

layout(std140, binding = 0) uniform Camera
{
	mat4 ViewProjection;
};

struct Attributes
{
	vec4 color;
	vec2 uv;
	vec3 normal;
};

layout(location = 0) out Attributes Attri_Output;

void main()
{
	Attri_Output.color = VertexColor;
	Attri_Output.uv =  Texcoord;
	Attri_Output.normal =  (Model * vec4(Normal, 0.0f)).xyz;
	gl_Position = ViewProjection * Model *  vec4(PositionOS, 1.0);
}


#program     fragment
#version 450 core

layout(std140, binding = 1) uniform LightData
{
	vec3 LightDirection ;
	vec4 LightColor ;
	float   LightIntensity ;
};
struct Attributes
{
	vec4 color;
	vec2 uv;
	vec3 normal;
};
layout(location = 0) in Attributes Attri_Input;

layout(location = 0) out vec4 FragColor;


uniform sampler2D MainTex;
void main()
{
	vec3 normalWS = normalize(Attri_Input.normal);
	vec3 lightDir = normalize(LightDirection);
	float NdotL =  max(dot(Attri_Input.normal, lightDir),0.0);
	float half_NdotL = (NdotL + 1.0f) * 0.5f;
	FragColor = vec4(texture(MainTex, Attri_Input.uv).rgb * NdotL,1.0) ;
}