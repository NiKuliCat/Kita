struct VSOutput
{
    float4 positionCS : SV_Position;
    float2 clipXY : TEXCOORD0;
};

struct PSOutput
{
    float4 color : SV_Target0;
    float depth : SV_Depth;
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
    output.clipXY = clipXY;
    return output;
}

float3 ReconstructWorldRayDirection(float2 clipXY)
{
    float4 clipPos = float4(clipXY, 1.0, 1.0);
    float4 viewPos = mul(gCamera.Matrix_I_P, clipPos);
    viewPos /= max(abs(viewPos.w), 1e-6);

    float3 viewDirVS = normalize(viewPos.xyz);
    float3x3 invViewRotation = (float3x3)gCamera.Matrix_I_V;
    return normalize(mul(invViewRotation, viewDirVS));
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
PSOutput PSMain(VSOutput input)
{
    PSOutput output;
    float3 cameraPos = gCamera.CameraPosWS.xyz;
    float3 rayDir = ReconstructWorldRayDirection(input.clipXY);

    if (abs(rayDir.y) <= 1e-5)
        discard;

    const float gridPlaneY = 0.0;
    float rayDistance = (gridPlaneY - cameraPos.y) / rayDir.y;
    if (rayDistance <= 0.0)
        discard;

    float3 worldPos = cameraPos + rayDir * rayDistance;
    float4 clipPos = mul(gCamera.Matrix_VP, float4(worldPos, 1.0));
    if (clipPos.w <= 1e-5)
        discard;

    float gridDepth = clipPos.z / clipPos.w;
    if (gridDepth <= 0.0 || gridDepth >= 1.0)
        discard;

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
    float planarDistance = distance(cameraPos.xz, worldPos.xz);
    float distanceFade = 1.0 - smoothstep(fadeStart, fadeEnd, planarDistance);
    distanceFade *= distanceFade;

    float angleFadeRange = max(gGrid.FadeParams.z, 1e-4);
    float angleFade = smoothstep(0.0, angleFadeRange, abs(rayDir.y));
    angleFade *= angleFade;

    float minorWeight = minor * (1.0 - major);
    float majorWeight = major;
    float axisWeight = max(axisX, axisZ);

    float4 color = gGrid.MinorColor * minorWeight + gGrid.MajorColor * majorWeight;
    color = lerp(color, gGrid.AxisXColor, axisX);
    color = lerp(color, gGrid.AxisZColor, axisZ);

    color.a *= distanceFade * angleFade;

    if (color.a <= 1e-4 && axisWeight <= 1e-4 && majorWeight <= 1e-4 && minorWeight <= 1e-4)
        discard;

    output.color = color;
    output.depth = gridDepth;
    return output;
}
