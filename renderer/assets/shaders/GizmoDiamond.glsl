#program vertex
#version 450 core

layout(location = 0) in vec3 PositionOS;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aRadius;
layout(location = 3) in int Index;

uniform mat4 Matrix_M;

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

layout(location = 0) out vec4 vColor;
layout(location = 1) out vec2 vCenterSS;
layout(location = 2) out float vRadiusPx;
layout(location = 3) out flat int vIndex;

void main()
{
    vec4 clip = Matrix_VP * Matrix_M * vec4(PositionOS, 1.0);
    gl_Position = clip;

    float radiusPx = max(aRadius, 0.5);
    gl_PointSize = radiusPx * 2.0;

    vec2 ndc = clip.xy / max(clip.w, 1e-6);
    vCenterSS = ndc * 0.5 + 0.5;
    vRadiusPx = radiusPx;
    vColor = aColor;
    vIndex = Index;
}

#program fragment
#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 1) in vec2 vCenterSS;
layout(location = 2) in float vRadiusPx;
layout(location = 3) flat in int vIndex;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int IDColor;
layout(location = 2) out int IndexColor;

layout(std140, binding = 2) uniform ScreenData
{
    vec4 ScreenSize;
};

uniform int id;

void main()
{
    vec2 centerPx = vCenterSS * ScreenSize.xy;
    vec2 d = abs(gl_FragCoord.xy - centerPx);
    float manhattan = d.x + d.y;

    float edge = 1.0;
    float alpha = 1.0 - smoothstep(vRadiusPx - edge, vRadiusPx + edge, manhattan);

    if (alpha <= 0.0)
        discard;

    FragColor = vec4(vColor.rgb, vColor.a * alpha);

    IDColor = (manhattan <= vRadiusPx - 1.0) ? id : -1;
    IndexColor = (manhattan <= vRadiusPx - 1.0) ? vIndex : -1;
}
