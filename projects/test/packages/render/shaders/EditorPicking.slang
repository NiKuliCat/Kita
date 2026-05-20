struct VSInput
{
    float3 position : ATTRIBUTE0;
    float4 color : ATTRIBUTE1;
    float2 texcoords : ATTRIBUTE2;
    float3 normal : ATTRIBUTE3;
    float3 tangent : ATTRIBUTE4;
    float3 bitangent : ATTRIBUTE5;
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

struct ObjectData
{
    float4x4 Matrix_M;
    float4x4 Matrix_I_M;
};

struct PickingPushConstants
{
    ObjectData PerObject;
    uint PickID;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneCameraData> u_Camera;

[[vk::binding(1, 0)]]
ConstantBuffer<SceneDirectionalLightData> u_MainLight;

[[vk::push_constant]]
PickingPushConstants g_PushConstants;

struct VSOutput
{
    float4 position : SV_Position;
};

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(g_PushConstants.PerObject.Matrix_M, float4(input.position, 1.0));
    output.position = mul(u_Camera.Matrix_VP, worldPos);
    return output;
}

[shader("fragment")]
uint PSMain(VSOutput input) : SV_Target0
{
    return g_PushConstants.PickID;
}