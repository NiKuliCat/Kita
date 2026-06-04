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

struct PSOutput
{
    float4 baseColorOpacity : SV_Target0;
    float4 normal : SV_Target1;
    float4 material : SV_Target2;
    float4 emissive : SV_Target3;
    float4 customData : SV_Target4;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneCameraData> g_Camera;

[[vk::binding(1, 0)]]
ConstantBuffer<SceneDirectionalLightData> g_MainLight;

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

    float4 albedoSample = _Albedo.Sample(input.uv);
    float4 metallicRoughnessSample = _MetallicRoughness.Sample(input.uv);
    float aoSample = _AmbientOcclusionTexture.Sample(input.uv).r;
    float opacitySample = _OpacityTexture.Sample(input.uv).r;

    float3 tangentNormal = _Normal.Sample(input.uv).xyz * 2.0 - 1.0;
    tangentNormal.xy *= _NormalScale;
    tangentNormal = SafeNormalize(tangentNormal, float3(0.0, 0.0, 1.0));

    float3 N = SafeNormalize(input.normalWS, float3(0.0, 1.0, 0.0));
    float3 T = SafeNormalize(
        input.tangentWS - N * dot(input.tangentWS, N),
        ComputeFallbackTangent(N));
    float3 B = SafeNormalize(
        input.bitangentWS - N * dot(input.bitangentWS, N) - T * dot(input.bitangentWS, T),
        cross(N, T));
    float3 normalWS = SafeNormalize(T * tangentNormal.x + B * tangentNormal.y + N * tangentNormal.z, N);

    float4 baseColor = albedoSample * _BaseColor * input.color;
    float opacity = saturate(baseColor.a * opacitySample * _Opacity);
    if (opacity <= _AlphaCutoff)
        discard;

    float metallic = saturate(_Metallic * metallicRoughnessSample.b);
    float roughness = saturate(_Roughness * metallicRoughnessSample.g);
    float ambientOcclusion = saturate(lerp(1.0, aoSample, _AmbientOcclusion));
    float3 emissive = _Emissive * _EmissionMask.Sample(input.uv).rgb;

    output.baseColorOpacity = float4(baseColor.rgb, opacity);
    output.normal = float4(normalWS * 0.5 + 0.5, 1.0);
    output.material = float4(metallic, roughness, ambientOcclusion, (float)KITA_SHADING_MODEL_ID);
    output.emissive = float4(emissive, saturate(_ToonThreshold));
    output.customData = float4(saturate(_SpecBand), 0.0, 0.0, 0.0);
    return output;
}
