
import render_core;

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
    float3 positionWS : TEXCOORD0;
    float3 normalWS : TEXCOORD1;
    float3 tangentWS : TEXCOORD2;
    float3 bitangentWS : TEXCOORD3;
    float2 uv : TEXCOORD4;
    float4 color : COLOR0;
};


struct MaterialGpuParams
{
    float4 BaseColor;
    float4 Emissive;
    float4 SurfaceParams;
    float4 MiscParams;
    float4 TextureFlags0;
    float4 TextureFlags1;
};



struct PSOutput
{
    float4 baseColorOpacity : SV_Target0;
    float4 normal : SV_Target1;
    float4 material : SV_Target2;
    float4 emissive : SV_Target3;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneCameraData> g_Camera;

[[vk::binding(1, 0)]]
ConstantBuffer<SceneDirectionalLightData> g_MainLight;

[[vk::binding(0, 1)]]
Sampler2D g_AlbedoTexture;

[[vk::binding(1, 1)]]
ConstantBuffer<MaterialGpuParams> g_Material;

[[vk::binding(2, 1)]]
Sampler2D g_NormalTexture;

[[vk::binding(3, 1)]]
Sampler2D g_MetallicRoughnessTexture;

[[vk::binding(4, 1)]]
Sampler2D g_AmbientOcclusionTexture;

[[vk::binding(5, 1)]]
Sampler2D g_EmissiveTexture;

[[vk::binding(6, 1)]]
Sampler2D g_OpacityTexture;

[[vk::push_constant]]
ObjectData g_Object;



[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 positionOS = float4(input.position, 1.0);
    float4 positionWS = mul(g_Object.Matrix_M, positionOS);

    float3x3 world3x3 = (float3x3)g_Object.Matrix_M;
    float3x3 normalMatrix = transpose((float3x3)g_Object.Matrix_I_M);

    float3 normalWS = SafeNormalize(mul(normalMatrix, input.normal), float3(0.0, 1.0, 0.0));
    float3 tangentWSRaw = mul(world3x3, input.tangent);
    float3 tangentWS = SafeNormalize(
        tangentWSRaw - normalWS * dot(tangentWSRaw, normalWS),
        ComputeFallbackTangent(normalWS));
    float3 bitangentRefWS = mul(world3x3, input.bitangent);
    float bitangentSign = dot(cross(normalWS, tangentWS), bitangentRefWS) < 0.0 ? -1.0 : 1.0;
    float3 bitangentWS = SafeNormalize(
        cross(normalWS, tangentWS) * bitangentSign,
        SafeNormalize(bitangentRefWS, float3(0.0, 0.0, 1.0)));
    
    output.positionCS = mul(g_Camera.Matrix_VP, positionWS);
    output.positionWS = positionWS.xyz;
    output.normalWS = normalWS;
    output.tangentWS = tangentWS;
    output.bitangentWS = bitangentWS;
    output.uv = input.texcoords;
    output.color = input.color;
    return output;
}

[shader("fragment")]
PSOutput PSMain(VSOutput input)
{
    PSOutput output;

    float4 albedoSample = g_AlbedoTexture.Sample(input.uv);
    float4 opacitySample = g_OpacityTexture.Sample(input.uv);
    float4 metallicRoughnessSample = g_MetallicRoughnessTexture.Sample(input.uv);
    float aoSample = g_AmbientOcclusionTexture.Sample(input.uv).r;

    float3 tangentNormal = g_NormalTexture.Sample(input.uv).xyz * 2.0 - 1.0;
    tangentNormal.xy *= g_Material.MiscParams.x;
    tangentNormal = SafeNormalize(tangentNormal, float3(0.0, 0.0, 1.0));


    float3 N = SafeNormalize(input.normalWS, float3(0.0, 1.0, 0.0));
    float3 T = SafeNormalize(
        input.tangentWS - N * dot(input.tangentWS, N),
        ComputeFallbackTangent(N));
    float3 B = SafeNormalize(
        input.bitangentWS - N * dot(input.bitangentWS, N) - T * dot(input.bitangentWS, T),
        cross(N, T));
    float3 normalWS = SafeNormalize(T * tangentNormal.x + B * tangentNormal.y + N * tangentNormal.z, N);

    float4 baseColor = albedoSample * g_Material.BaseColor * input.color;
    float opacity = saturate(baseColor.a * opacitySample.r * g_Material.SurfaceParams.w);
    if (opacity <= g_Material.MiscParams.y)
        discard;

    float metallic = saturate(g_Material.SurfaceParams.x * metallicRoughnessSample.b);
    float roughness = saturate(g_Material.SurfaceParams.y * metallicRoughnessSample.g);
    float ambientOcclusion = saturate(g_Material.SurfaceParams.z * aoSample);

    float3 emissiveFactor = g_Material.TextureFlags1.x > 0.5
        ? g_EmissiveTexture.Sample(input.uv).rgb
        : float3(1.0, 1.0, 1.0);
    float3 emissive = g_Material.Emissive.rgb * emissiveFactor;

    output.baseColorOpacity = float4(baseColor.rgb, opacity);
    output.normal = float4(normalWS * 0.5 + 0.5, 1.0);
    output.material = float4(metallic, roughness, ambientOcclusion, 1.0);
    output.emissive = float4(emissive, 1.0);
    return output;
}
