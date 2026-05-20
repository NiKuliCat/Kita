module render_core;


public struct SceneCameraData
{
   public float4x4 Matrix_V;
   public float4x4 Matrix_P;
   public float4x4 Matrix_VP;
   public float4x4 Matrix_I_V;
   public float4x4 Matrix_I_P;
   public float4x4 Matrix_I_VP;
   public float4 CameraPosWS;
};


public struct ObjectData
{
   public float4x4 Matrix_M;
   public float4x4 Matrix_I_M;
};

public struct SceneDirectionalLightData
{
   public float4 Direction;
   public float4 Color;
};





public float3 SafeNormalize(float3 value, float3 fallbackValue)
{
    float lengthSq = dot(value, value);
    if (lengthSq <= 1e-8)
        return fallbackValue;

    return value * rsqrt(lengthSq);
}

public float3 ComputeFallbackTangent(float3 normalWS)
{
    float3 referenceAxis = abs(normalWS.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    return SafeNormalize(cross(referenceAxis, normalWS), float3(1.0, 0.0, 0.0));
}

