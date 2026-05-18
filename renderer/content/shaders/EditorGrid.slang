struct VSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
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
Texture2D<float> gDepthTexture;

[[vk::binding(0, 1)]]
SamplerState gDepthSampler;

struct GridPushConstants
{
    float4 MinorColor;
    float4 MajorColor;
    float4 AxisXColor;
    float4 AxisZColor;
    float4 GridParams;
    float4 FadeParams;
};

[[vk::push_constant]]
ConstantBuffer<GridPushConstants> gGrid;

float2 FullscreenTriangleUV(uint vertexID)
{
    if (vertexID == 0) return float2(0.0, 0.0);
    if (vertexID == 1) return float2(2.0, 0.0);
    return float2(0.0, 2.0);
}

[shader("vertex")]
VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    float2 uv = FullscreenTriangleUV(vertexID);
    output.uv = uv;
    output.position = float4(uv * float2(2.0, 2.0) + float2(-1.0, -1.0), 0.0, 1.0);
    return output;
}

float3 ReconstructWorldPosition(float2 uv, float depth)
{
    float4 ndc = float4(uv * float2(2.0, 2.0) - 1.0, depth, 1.0);
    float4 world = mul(gCamera.Matrix_I_VP, ndc);
    return world.xyz / max(world.w, 1e-6);
}

float GridCoverage(float2 worldXZ, float cellSize, float lineWidthPx)
{
    float2 grid = worldXZ / cellSize;
    float2 gridDeriv = max(fwidth(grid), float2(1e-5, 1e-5));
    float2 cell = abs(frac(grid - 0.5) - 0.5) / gridDeriv;
    float line = min(cell.x, cell.y);
    float halfWidth = max(lineWidthPx * 0.5, 0.5);
    return saturate(halfWidth - line);
}

float AxisCoverage(float axisCoord, float axisWidthPx, float derivative)
{
    float safeDerivative = max(derivative, 1e-5);
    float distanceToAxis = abs(axisCoord) / safeDerivative;
    float halfWidth = max(axisWidthPx * 0.5, 0.5);
    return saturate(halfWidth - distanceToAxis);
}

[shader("fragment")]
float4 PSMain(VSOutput input) : SV_Target0
{
    // The renderer applies Vulkan Y-flip in the viewport state. For fullscreen
    // passes, the interpolated triangle UV therefore needs to be flipped back
    // to sample the depth texture in the same screen-space orientation.
    float2 uv = float2(input.uv.x, 1.0 - input.uv.y);
    float sceneDepth = gDepthTexture.SampleLevel(gDepthSampler, uv, 0.0);

    if (sceneDepth <= 0.0 || sceneDepth >= 1.0)
        discard;

    float3 worldPos = ReconstructWorldPosition(uv, sceneDepth);

    float minorCellSize = max(gGrid.GridParams.x, 1e-4);
    float majorCellSize = max(gGrid.GridParams.y, minorCellSize);
    float minorWidthPx = gGrid.GridParams.z;
    float majorWidthPx = gGrid.GridParams.w;

    float minor = GridCoverage(worldPos.xz, minorCellSize, minorWidthPx);
    float major = GridCoverage(worldPos.xz, majorCellSize, majorWidthPx);

    float2 derivWS = max(fwidth(worldPos.xz), float2(1e-5, 1e-5));
    float axisX = AxisCoverage(worldPos.z, 1.75, derivWS.y);
    float axisZ = AxisCoverage(worldPos.x, 1.75, derivWS.x);

    float fadeStart = gGrid.FadeParams.x;
    float fadeEnd = max(gGrid.FadeParams.y, fadeStart + 1.0);
    float distanceFade = 1.0 - smoothstep(fadeStart, fadeEnd, distance(gCamera.CameraPosWS.xyz, worldPos));

    float viewDotUp = abs(dot(normalize(worldPos - gCamera.CameraPosWS.xyz), float3(0.0, 1.0, 0.0)));
    float angleFade = saturate(viewDotUp + gGrid.FadeParams.z);

    float minorWeight = minor * (1.0 - major);
    float majorWeight = major;
    float axisWeight = max(axisX, axisZ);

    float4 color = gGrid.MinorColor * minorWeight + gGrid.MajorColor * majorWeight;
    color = lerp(color, gGrid.AxisXColor, axisX);
    color = lerp(color, gGrid.AxisZColor, axisZ);

    color.a *= distanceFade * angleFade;

    if (color.a <= gGrid.FadeParams.w && axisWeight <= 0.0 && majorWeight <= 0.0 && minorWeight <= 0.0)
        discard;

    return color;
}
