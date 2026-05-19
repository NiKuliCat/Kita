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
    float4 position : SV_Position;
    float4 color : COLOR0;
};

[shader("vertex")]
VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}

struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

[shader("fragment")]
float4 PSMain(PSInput input) : SV_Target0
{
    return input.color;
}
