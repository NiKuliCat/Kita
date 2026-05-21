import render_core;
import pbr_common;
import IBL;

struct VSOutput
{
    float4 positionCS : SV_Position;
    float2 uv : TEXCOORD0;
};

struct PSOutput
{
    float4 color : SV_Target0;
    float depth : SV_Depth;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneCameraData> gCamera;

[[vk::binding(1, 0)]]
ConstantBuffer<SceneDirectionalLightData> gMainLight;

[[vk::binding(0, 1)]]
Sampler2D g_GBufferBaseColorOpacity;

[[vk::binding(1, 1)]]
Sampler2D g_GBufferNormal;

[[vk::binding(2, 1)]]
Sampler2D g_GBufferMaterial;

[[vk::binding(3, 1)]]
Sampler2D g_GBufferEmissive;

[[vk::binding(4, 1)]]
Sampler2D g_GBufferDepth;

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

float3 DecodeNormal(float3 encodedNormal)
{
    return normalize(encodedNormal * 2.0 - 1.0);
}

float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float2 clipXY = uv * 2.0 - 1.0;
    clipXY.y = -clipXY.y;

    float4 clipPos = float4(clipXY, depth, 1.0);
    float4 positionWS = mul(gCamera.Matrix_I_VP, clipPos);
    return positionWS.xyz / max(abs(positionWS.w), 1e-6);
}

[shader("fragment")]
PSOutput PSMain(VSOutput input)
{
    PSOutput output;

    float4 baseColorOpacity = g_GBufferBaseColorOpacity.Sample(input.uv);
    float4 normalSample = g_GBufferNormal.Sample(input.uv);
    float4 materialSample = g_GBufferMaterial.Sample(input.uv);
    float4 emissiveSample = g_GBufferEmissive.Sample(input.uv);
    float depth = g_GBufferDepth.Sample(input.uv).r;

    if (depth >= 1.0)
    {
        output.color = float4(emissiveSample.rgb, 1.0);
        output.depth = 1.0;
        return output;
    }

    float3 albedo = baseColorOpacity.rgb;
    float roughness = materialSample.g;
    float metallic = materialSample.r;
    float ambientOcclusion = 1.0;

    float clampedMetallic = saturate(metallic);
    float clampedRoughness = ClampRoughness(roughness);

    float3 F0 = ComputeF0(albedo, clampedMetallic);

    float3 N = DecodeNormal(normalSample.rgb);
    float3 positionWS = ReconstructWorldPosition(input.uv, depth);
    float3 L = SafeNormalize(-gMainLight.Direction.xyz, float3(0.0, 1.0, 0.0));
    float3 V = SafeNormalize(gCamera.CameraPosWS.xyz - positionWS, float3(0.0, 0.0, 1.0));

    float intensity = gMainLight.Color.w;
    float3 lightColor = gMainLight.Color.rgb;

    float3 directLighting = EvaluateDirectionalLightPBR(albedo,clampedMetallic,clampedRoughness,ambientOcclusion,N,V,L,lightColor,intensity);

    float3 F = FresnelSchlickRoughness(saturate(dot(N, V)), F0, clampedRoughness);
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - clampedMetallic);

    float3 diffuseIBL = EvaluateDiffuseIBL(N, albedo, clampedMetallic);
    float3 specularIBL = EvaluateSpecularIBL(N, V, clampedRoughness, F0);


    float3 indirectLighting = (kD * diffuseIBL + specularIBL) * ambientOcclusion;

    output.color = float4(directLighting + indirectLighting + emissiveSample.rgb, 1.0);
    output.depth = depth;
    return output;
}
