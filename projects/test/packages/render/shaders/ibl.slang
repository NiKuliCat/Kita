module IBL;
import pbr_common;

[[vk::binding(5, 1)]]
public SamplerCube g_IrradianceCube;

[[vk::binding(6, 1)]]
public SamplerCube g_PrefilteredSpecularCube;

[[vk::binding(7, 1)]]
public Sampler2D g_BrdfLut;

[[vk::binding(8, 1)]]
public SamplerCube g_EnvironmentCube;

public float3 EvaluateDiffuseIBL(float3 N,float3 baseColor,float metallic)
{
    float3 irradiance = g_IrradianceCube.Sample(N).rgb;
    float3 diffuseColor = baseColor * (1.0 - saturate(metallic));
    return irradiance * diffuseColor;
}

public float3 EvaluateSpecularIBL(float3 N,float3 V,float roughness,float3 F0)
{
    float3 R = reflect(-V, N);

    const float prefilterMipCount = 8.0;
    float mipLevel = ClampRoughness(roughness) * (prefilterMipCount - 1.0);

    float3 prefilteredColor = g_PrefilteredSpecularCube.SampleLevel(R, mipLevel).rgb;
    float NdotV = saturate(dot(N, V));
    float2 envBrdf = g_BrdfLut.Sample(float2(NdotV, ClampRoughness(roughness))).rg;
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

    return prefilteredColor * (F * envBrdf.x + envBrdf.y);
}