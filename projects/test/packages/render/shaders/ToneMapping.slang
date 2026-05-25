import render_core;

struct VSOutput
{
    float4 positionCS : SV_Position;
    float2 uv : TEXCOORD0;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneCameraData> gCamera;

[[vk::binding(1, 0)]]
ConstantBuffer<SceneDirectionalLightData> gMainLight;

[[vk::binding(0, 1)]]
Sampler2D g_HDRSource;

float2 FullscreenTrianglePosition(uint vertexID)
{
    if (vertexID == 0) return float2(-1.0, -1.0);
    if (vertexID == 1) return float2(-1.0, 3.0);
    return float2(3.0, -1.0);
}

[shader("vertex")]
VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    float2 clipXY = FullscreenTrianglePosition(vertexID);
    output.positionCS = float4(clipXY, 0.0, 1.0);
    output.uv = clipXY * float2(0.5, -0.5) + 0.5;
    return output;
}

float3 ACESFilm(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[shader("fragment")]
float4 PSMain(VSOutput input) : SV_Target0
{
    float3 hdrColor = max(g_HDRSource.Sample(input.uv).rgb, 0.0.xxx);
    float exposure = 1.0;
    float3 mapped = ACESFilm(hdrColor * exposure);
    return float4(mapped, 1.0);
}
