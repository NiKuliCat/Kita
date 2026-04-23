#program vertex
#version 450 core
layout(location = 0) in vec3 PositionOS;

layout(location = 0) out vec2 vNdc;

void main()
{
    vNdc = PositionOS.xy;
    gl_Position = vec4(PositionOS.xy, 0.0, 1.0);
}

#program fragment
#version 450 core

layout(std140, binding = 0) uniform CameraData
{
	mat4 Matrix_V;
	mat4 Matrix_P;
	mat4 Matrix_VP;
	mat4 Matrix_I_V;
	mat4 Matrix_I_P;
	mat4 Matrix_I_VP;
	vec4 CameraPosWS;
};

layout(std140, binding = 10) uniform EditorGrid
{
    vec4 Param0;          // x CellSize, y MajorStep, z FadeStart, w FadeEnd
    vec4 Param1;          // x MinorW(px), y MajorW(px), z AxisW(px), w reserved
    vec4 MinorColor;
    vec4 MajorColor;
    vec4 AxisXColor;
    vec4 AxisZColor;
};

layout(location = 0) in vec2 vNdc;
layout(location = 0) out vec4 FragColor;
layout(location = 1) out int IDColor;

float GridLineAA(vec2 coord, float widthPx)
{
    vec2 fw = max(fwidth(coord), vec2(1e-5));
    vec2 d2 = abs(fract(coord - 0.5) - 0.5) / fw;
    float d = min(d2.x, d2.y);
    return 1.0 - smoothstep(widthPx, widthPx + 1.0, d);
}

float AxisLineAA(float coord, float widthPx)
{
    float fw = max(fwidth(coord), 1e-5);
    float d = abs(coord) / fw;
    return 1.0 - smoothstep(widthPx, widthPx + 1.0, d);
}

void main()
{
    float cellSize  = max(Param0.x, 1e-4);
    float majorStep = max(Param0.y, 1.0);
    float fadeStart = Param0.z;
    float fadeEnd   = max(Param0.w, fadeStart + 1e-4);

    float minorW = Param1.x;
    float majorW = Param1.y;
    float axisW  = Param1.z;
    float gridY  = CameraPosWS.w;

    // 从屏幕射线求与 y=gridY 平面的交点
    vec4 near4 = Matrix_I_VP * vec4(vNdc, -1.0, 1.0);
    vec4 far4  = Matrix_I_VP * vec4(vNdc,  1.0, 1.0);
    vec3 p0 = near4.xyz / near4.w;
    vec3 p1 = far4.xyz  / far4.w;
    vec3 dir = p1 - p0;

    if (abs(dir.y) < 1e-6) discard;

    float t = (gridY - p0.y) / dir.y;
    if (t <= 0.0) discard;

    vec3 worldPos = p0 + dir * t;

    // 距离淡出
    float camDist = distance(worldPos, CameraPosWS.xyz);
    float fade = 1.0 - smoothstep(fadeStart, fadeEnd, camDist);
    if (fade <= 0.001) discard;

    // 网格线
    vec2 minorCoord = worldPos.xz / cellSize;
    vec2 majorCoord = worldPos.xz / (cellSize * majorStep);

    float majorA = GridLineAA(majorCoord, majorW);
    float minorA = GridLineAA(minorCoord, minorW) * (1.0 - majorA);

    // 轴线：z=0 是 X 轴线；x=0 是 Z 轴线
    float axisX = AxisLineAA(worldPos.z, axisW);
    float axisZ = AxisLineAA(worldPos.x, axisW);

    vec4 color = MinorColor * minorA + MajorColor * majorA;
    float alpha = max( MinorColor.a * minorA,  MajorColor.a * majorA);

    color = mix(color, AxisXColor, axisX);
    alpha = max(alpha, axisX);

    color = mix(color, AxisZColor, axisZ);
    alpha = max(alpha, axisZ);

    alpha *= fade;

    // 写入正确深度，和场景深度一致
    vec4 clip = Matrix_VP * vec4(worldPos, 1.0);
    float depth = clip.z / clip.w * 0.5 + 0.5;
    if (depth <= 0.0 || depth >= 1.0) discard;
    gl_FragDepth = depth;

    FragColor = vec4(color.rgb, alpha);
    IDColor = -1;
}
