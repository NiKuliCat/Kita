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
    float3 normalWS : TEXCOORD0;
    float3 positionWS : TEXCOORD1;
};

[[vk::binding(0, 0)]]
ConstantBuffer<SceneCameraData> g_Camera;

[[vk::binding(1, 0)]]
ConstantBuffer<SceneDirectionalLightData> g_MainLight;

[[vk::push_constant]]
ObjectData g_Object;

float3 ACESApprox(float3 color)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float3 LinearToSRGB(float3 color)
{
    float3 positiveColor = max(color, 0.0.xxx);
    float3 cutoff = step(float3(0.0031308, 0.0031308, 0.0031308), positiveColor);
    float3 lower = positiveColor * 12.92;
    float3 higher = 1.055 * pow(positiveColor, 1.0 / 2.4) - 0.055;
    return lerp(lower, higher, cutoff);
}

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput output;

    float4 positionWS = mul(g_Object.Matrix_M, float4(input.position, 1.0));
    float3x3 normalMatrix = transpose((float3x3)g_Object.Matrix_I_M);

    output.positionCS = mul(g_Camera.Matrix_VP, positionWS);
    output.positionWS = positionWS.xyz;
    output.normalWS = normalize(mul(normalMatrix, input.normal));
    return output;
}

[shader("fragment")]
float4 PSMain(VSOutput input) : SV_Target0
{
    float3 normalWS = normalize(input.normalWS);
    float3 viewDir = normalize(g_Camera.CameraPosWS.xyz - input.positionWS);
    float3 sampleDir = reflect(-viewDir, normalWS);

    float3 envColor = max(_Albedo.Sample(sampleDir).rgb, 0.0.xxx);
    float fresnel = pow(1.0 - saturate(dot(normalWS, viewDir)), 5.0);
    float3 color = lerp(envColor, envColor * 1.08, fresnel);

    color *= max(_BaseColor.rgb, float3(0.001, 0.001, 0.001));
    color = ACESApprox(color);
    return float4(LinearToSRGB(color), 1.0);
}
