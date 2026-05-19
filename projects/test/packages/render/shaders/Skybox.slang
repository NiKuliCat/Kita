struct VSOutput
{
    float4 positionCS : SV_Position;
    float3 viewDirWS : TEXCOORD0;
};

struct SceneCameraData
{
    float4x4 Matrix_V;
    float4x4 Matrix_P;
    float4x4 Matrix_VP;
    float4x4 Matrix_I_V;
    float4x4 Matrix_I_P;
    float4x4 Matrix_I_VP;
    float4 CameraPosWS;
};

struct SceneDirectionalLightData
{
    float4 Direction;
    float4 Color;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneCameraData> gCamera;

[[vk::binding(1, 0)]]
ConstantBuffer<SceneDirectionalLightData> gMainLight;

[[vk::binding(0, 1)]]
SamplerCube gSkyboxTexture;

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
    output.positionCS = float4(clipXY, 1.0, 1.0);

    float4 clipPos = float4(clipXY, 1.0, 1.0);
    float4 viewPos = mul(gCamera.Matrix_I_P, clipPos);
    viewPos /= max(abs(viewPos.w), 1e-6);

    float3 viewDirVS = normalize(viewPos.xyz);
    float3x3 invViewRotation = (float3x3)gCamera.Matrix_I_V;
    output.viewDirWS = normalize(mul(invViewRotation, viewDirVS));
    return output;
}

[shader("fragment")]
float4 PSMain(VSOutput input) : SV_Target0
{
    float3 direction = normalize(input.viewDirWS);
    float3 color = gSkyboxTexture.Sample(direction).rgb;
    return float4(color, 1.0);
}
