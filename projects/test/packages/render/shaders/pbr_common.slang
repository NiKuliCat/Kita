module pbr_common;

import render_core;

// PBR 常量
public static const float PI = 3.14159265359;
public static const float INV_PI = 0.31830988618;
public static const float PBR_EPSILON = 1e-4;

// 平方运算，便于统一表达式书写
public float Pow2(float x)
{
    return x * x;
}

// 五次方运算，Fresnel Schlick 近似会频繁使用
public float Pow5(float x)
{
    float x2 = x * x;
    return x2 * x2 * x;
}

// 限制粗糙度下限，避免极小 roughness 导致高光异常尖锐或数值不稳定
public float ClampRoughness(float roughness)
{
    return clamp(roughness, 0.08, 1.0);
}

// Schlick Fresnel 近似
// 用于根据观察角度近似计算菲涅耳反射
public float3 FresnelSchlick(float cosTheta, float3 F0)
{
    float t = saturate(1.0 - cosTheta);
    return F0 + (1.0 - F0) * Pow5(t);
}

// 带 roughness 修正的 Fresnel
// 这个版本通常用于 IBL 阶段，能让高粗糙度材质边缘反射更稳定
public float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 oneMinusRoughness = float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness);
    float3 maxTerm = max(oneMinusRoughness, F0);
    float t = saturate(1.0 - cosTheta);
    return F0 + (maxTerm - F0) * Pow5(t);
}

// GGX / Trowbridge-Reitz 法线分布函数 D
// 决定微表面高光的分布形状，是现代实时 PBR 的主流选择
public float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = ClampRoughness(roughness);
    float a2 = a * a;
    float a4 = a2 * a2;

    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a4 - 1.0) + 1.0;
    return a4 / max(PI * denom * denom, PBR_EPSILON);
}

// Schlick-GGX 单边几何项
// NdotX 可以传 NdotV 或 NdotL
public float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = ClampRoughness(roughness) + 1.0;
    float k = (r * r) * 0.125;

    return NdotX / max(NdotX * (1.0 - k) + k, PBR_EPSILON);
}

// Smith 几何项
// 同时考虑视线方向和光线方向带来的微表面遮蔽效应
public float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));

    float ggxV = GeometrySchlickGGX(NdotV, roughness);
    float ggxL = GeometrySchlickGGX(NdotL, roughness);
    return ggxV * ggxL;
}

// 根据 metallic workflow 计算 F0
// 非金属通常使用 0.04 左右的基础反射率，金属则直接使用 baseColor
public float3 ComputeF0(float3 baseColor, float metallic)
{
    float3 dielectricF0 = float3(0.04, 0.04, 0.04);
    return lerp(dielectricF0, baseColor, saturate(metallic));
}

// Lambert 漫反射
// 当前阶段足够稳定，后续如果要更贴近 UE 的能量模型再细化
public float3 DiffuseLambert(float3 diffuseColor)
{
    return diffuseColor * INV_PI;
}

// 评估 GGX 镜面项
// 返回 Cook-Torrance BRDF 中的 specular 部分
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

// 评估单个方向光的完整 PBR 结果
// 当前 deferred lighting 阶段可以直接调用这个入口函数
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
    float ao = saturate(ambientOcclusion);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    if (NdotL <= 0.0 || NdotV <= 0.0)
        return 0.0.xxx;

    // 计算基础反射率
    float3 F0 = ComputeF0(baseColor, clampedMetallic);

    // 半程向量
    float3 H = SafeNormalize(V + L, N);

    // 菲涅耳项
    float3 F = FresnelSchlick(saturate(dot(H, V)), F0);

    // 金属几乎没有漫反射，非金属的漫反射主要由 baseColor 提供
    float3 diffuseColor = baseColor * (1.0 - clampedMetallic);

    // 能量守恒：分给镜面越多，漫反射就越少
    float3 kD = (1.0 - F) * (1.0 - clampedMetallic);

    float3 diffuse = DiffuseLambert(diffuseColor) * kD;
    float3 specular = SpecularGGX(N, V, L, clampedRoughness, F0);

    float3 radiance = lightColor * lightIntensity;

    return (diffuse + specular) * radiance * NdotL ;
}
