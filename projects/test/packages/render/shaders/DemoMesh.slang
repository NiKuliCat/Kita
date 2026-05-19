struct VSInput
{
    float3 position : ATTRIBUTE0;
    float4 color : ATTRIBUTE1;
    float2 texcoords : ATTRIBUTE2;
    float3 normal : ATTRIBUTE3;
    float3 tangent : ATTRIBUTE4;
    float3 bitangent : ATTRIBUTE5;
};

struct VSOutput
{
    float4 positionCS : SV_Position;
    float3 normalWS : TEXCOORD0;
    float2 uv       : TEXCOORD1;
};

[[vk::binding(0, 0)]]
cbuffer CameraBuffer
{
    float4x4 Matrix_V;
    float4x4 Matrix_P;
    float4x4 Matrix_VP;
    float4x4 Matrix_I_V;
    float4x4 Matrix_I_P;
    float4x4 Matrix_I_VP;
    float4 CameraPosWS;
};

[[vk::binding(1, 0)]]
cbuffer DirLightBuffer
{
    float4 Direction;
    float4 ColorAndIntensity;
};

[[vk::binding(0, 1)]]
Sampler2D g_AlbedoTexture;

struct PerObjectData
{
    float4x4 Matrix_M;
    float4x4 Matrix_I_M;
};

[[vk::push_constant]]
PerObjectData g_Object;

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 positionOS =  float4(input.position, 1.0);
    float4 positionWS =  mul(g_Object.Matrix_M, positionOS);
    float4 positionCS =  mul(Matrix_VP, positionWS);
    output.positionCS =  positionCS;
    output.normalWS   =  mul(g_Object.Matrix_M, float4(input.normal,0.0)).xyz;
    output.uv         = input.texcoords;
    return output;
}

struct PSInput
{
    float4 positionCS : SV_Position;
    float3 normalWS   : TEXCOORD0;
    float2 uv         : TEXCOORD1;
};

[shader("fragment")]
float4 PSMain(PSInput input) : SV_Target0
{
    float3 N = normalize(input.normalWS);
    float3 L = normalize(-Direction.xyz);
    float NdotL = dot(N,L);
    float3 color = g_AlbedoTexture.Sample(input.uv).rgb;
    return float4(color,1.0);
}


