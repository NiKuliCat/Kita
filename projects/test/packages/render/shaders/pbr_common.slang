module pbr_common;

import render_core;

// PBR 常量
public static const float PI = 3.14159265359;
public static const float INV_PI = 0.31830988618;
public static const float PBR_EPSILON = 1e-4;

// 平方运算，便于统一公式书写
public float Pow2(float x)
{
    return x * x;
}

// 五次方，Fresnel Schlick 近似里会频繁使用
public float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

// 限制 roughness 下限，避免极低粗糙度时数值过尖
public float ClampRoughness(float roughness)
{
    return clamp(roughness, 0.001, 1.0);
}

// Schlick Fresnel
public float3 FresnelSchlick(float cosTheta, float3 F0)
{
    float t = saturate(1.0 - cosTheta);
    return F0 + (1.0 - F0) * Pow5(t);
}

// 带 roughness 修正的 Fresnel，常用于 IBL
public float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 oneMinusRoughness = float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness);
    float3 maxTerm = max(oneMinusRoughness, F0);
    float t = saturate(1.0 - cosTheta);
    return F0 + (maxTerm - F0) * Pow5(t);
}


public float D_GGX(float NoH, float roughness) {
    float a = NoH * roughness;
    float k = roughness / (1.0 - NoH * NoH + a * a);
    return k * k * (1.0 / PI);
}
 
// GGX / Trowbridge-Reitz 法线分布项 D
public float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = ClampRoughness(roughness);
    float a2 = a * a;

    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, PBR_EPSILON);
}

// Schlick-GGX 几何项单边近似
public float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = ClampRoughness(roughness) + 1.0;
    float k = (r * r) * 0.125;

    return NdotX / max(NdotX * (1.0 - k) + k, PBR_EPSILON);
}

// Smith 几何项
public float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));

    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

// metallic workflow 下的基础反射率 F0
public float3 ComputeF0(float3 baseColor, float metallic)
{
    float3 dielectricF0 = float3(0.04, 0.04, 0.04);
    return lerp(dielectricF0, baseColor, saturate(metallic));
}

// Lambert 漫反射
public float3 DiffuseLambert(float3 diffuseColor)
{
    return diffuseColor * INV_PI;
}

// GGX 镜面 BRDF
public float3 SpecularGGX(
    float3 N,
    float3 V,
    float3 L,
    float roughness,
    float3 F0)
{
    float3 H = SafeNormalize(V + L, N);

    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float HdotV = saturate(dot(H, V));

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(HdotV, F0);

    float denom = max(4.0 * NdotV * NdotL, PBR_EPSILON);
    return (D * G * F) / denom;
}

// 直射光 PBR 评价函数
// 当前用于 DeferredLighting.slang 中的方向光直射部分。
public float3 EvaluateDirectionalLightPBR(
    float3 baseColor,
    float metallic,
    float roughness,
    float ambientOcclusion,
    float3 N,
    float3 V,
    float3 L,
    float3 lightColor,
    float lightIntensity)
{
    float clampedMetallic = saturate(metallic);
    float clampedRoughness = ClampRoughness(roughness);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    if (NdotL <= 0.0 || NdotV <= 0.0)
        return 0.0.xxx;

    float3 F0 = ComputeF0(baseColor, clampedMetallic);
    float3 H = SafeNormalize(V + L, N);
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

    float3 diffuseColor = baseColor * (1.0 - clampedMetallic);
    float3 kD = (1.0 - F) * (1.0 - clampedMetallic);

    float3 diffuse = DiffuseLambert(diffuseColor) * kD;
    float3 specular = SpecularGGX(N, V, L, clampedRoughness, F0);
    float3 radiance = lightColor * lightIntensity;

    // AO 不应该压直射光，所以这里不乘 ambientOcclusion。
    return (diffuse + specular) * radiance * NdotL;
}

// 间接光 IBL 汇总入口
// 用法类似 UE 的 split-sum specular IBL：
// 1. irradiance 负责漫反射间接光
// 2. prefilteredSpecular + brdfSample 负责镜面间接光
// 3. AO 只作用在间接光上
//
// 当前用于 DeferredLighting.slang 中的 IBL 合成阶段。
public float3 EvaluateIndirectIBL(
    float3 baseColor,
    float metallic,
    float roughness,
    float ambientOcclusion,
    float3 N,
    float3 V,
    float3 irradiance,
    float3 prefilteredSpecular,
    float2 brdfSample)
{
    float clampedMetallic = saturate(metallic);
    float clampedRoughness = ClampRoughness(roughness);
    float ao = saturate(ambientOcclusion);

    float NdotV = saturate(dot(N, V));
    float3 F0 = ComputeF0(baseColor, clampedMetallic);
    float3 F = FresnelSchlickRoughness(NdotV, F0, clampedRoughness);

    float3 diffuseColor = baseColor * (1.0 - clampedMetallic);
    float3 kD = (1.0 - F) * (1.0 - clampedMetallic);

    float3 diffuse = irradiance * diffuseColor * kD;
    float3 specular = prefilteredSpecular * (F * brdfSample.x + brdfSample.y);

    return (diffuse + specular) * ao;
}
