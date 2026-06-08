Material "Kita/Deferred/GBuffer"
{
    Domain = Surface

    Properties
    {
        _BaseColor("Base Color", Color) = (1.0, 1.0, 1.0, 1.0)
        _Emissive("Emissive", Float3) = (1.0, 1.0, 1.0)
        _Metallic("Metallic", Float) = 0.0
        _Roughness("Roughness", Float) = 1.0
        _AmbientOcclusion("Ambient Occlusion", Float) = 1.0
        _Opacity("Opacity", Float) = 1.0
        _NormalScale("Normal Scale", Float) = 1.0
        _AlphaCutoff("Alpha Cutoff", Float) = 0.0

        _Albedo("Albedo", Texture2D) = "white"
        _Normal("Normal", Texture2D) = "normal"
        _MetallicRoughness("Metallic Roughness", Texture2D) = "white"
        _AmbientOcclusionTexture("AO Texture", Texture2D) = "white"

        _EmissionMask("Emission Mask", Texture2D) = "black"

        _OpacityTexture("Opacity Texture", Texture2D) = "white"
    }

    Lighting
    {
        ShadingModel = DefaultLit
        CustomDataCount = 0
    }

    Pass "GBuffer"
    {
        LightMode = GBuffer

        RenderState
        {
            Cull = Off
            DepthTest = Less
            DepthWrite = On
            Blend = Off
        }

        Program
        {
            Source = "GBuffer.slang"
            Vertex = VSMain
            Fragment = PSMain
        }
    }

    
}
