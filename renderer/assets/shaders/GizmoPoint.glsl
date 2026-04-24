#program vertex
#version 450 core

layout(location = 0) in vec3 aPositionWS;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aRadius;
layout(location = 3) in int aID;

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
layout(location = 1) flat out int vID;
layout(location = 2) out vec2 vCenterSS;
layout(location = 3) out float vRadiusPx;

void main()
{
    vec4 clip = Matrix_VP * vec4(aPositionWS, 1.0);
    gl_Position = clip;

    float radiusPx = max(aRadius, 0.5);
    gl_PointSize = radiusPx * 2.0;

    vec2 ndc = clip.xy / max(clip.w, 1e-6);
    vCenterSS = ndc * 0.5 + 0.5;
    vRadiusPx = radiusPx;

    vColor = aColor;
    vID = aID;
}

#program fragment
#version 450 core

layout(location = 0) in vec4 vColor;
layout(location = 1) flat in int vID;
layout(location = 2) in vec2 vCenterSS;
layout(location = 3) in float vRadiusPx;

layout(location = 0) out vec4 FragColor;
layout(location = 1) out int IDColor;

layout(std140, binding = 2) uniform ScreenData
{

    vec4 ScreenSize;
};

void main()
{
    vec2 centerPx = vCenterSS * ScreenSize.xy;
    vec2 d = gl_FragCoord.xy - centerPx;
    float dist = length(d);

    float edge = 1.0;
    float alpha = 1.0 - smoothstep(vRadiusPx - edge, vRadiusPx + edge, dist);

    if (alpha <= 0.0)
        discard;

    FragColor = vec4(vColor.rgb, vColor.a * alpha);

    IDColor = (dist <= vRadiusPx - 1.0) ? vID : -1;
}
