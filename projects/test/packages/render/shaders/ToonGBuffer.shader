Shader "Kita/Deferred/ToonGBuffer"
{
    Properties
    {
        _BaseColor("Base Color", Color) = (1.0, 1.0, 1.0, 1.0)
        _Emissive("Emissive", Float3) = (0.0, 0.0, 0.0)
        _Metallic("Metallic", Float) = 0.0
        _Roughness("Roughness", Float) = 1.0
        _AmbientOcclusion("Ambient Occlusion", Float) = 1.0
        _Opacity("Opacity", Float) = 1.0
        _NormalScale("Normal Scale", Float) = 1.0
        _AlphaCutoff("Alpha Cutoff", Float) = 0.0
        _ToonThreshold("Toon Threshold", Float) = 0.5
        _SpecBand("Spec Band", Float) = 0.15

        _Albedo("Albedo", Texture2D) = "white"
        _Normal("Normal", Texture2D) = "normal"
        _MetallicRoughness("Metallic Roughness", Texture2D) = "white"
        _AmbientOcclusionTexture("AO Texture", Texture2D) = "white"
        _EmissionMask("Emission Mask", Texture2D) = "black"
        _OpacityTexture("Opacity Texture", Texture2D) = "white"
    }

    Lighting
    {
        ShadingModel = Toon
        CustomDataCount = 2
        Include = "ToonLightingHook.slang"
        Evaluate = EvaluateToonLighting
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
            Source = "ToonGBuffer.slang"
            Vertex = VSMain
            Fragment = PSMain
        }
    }
}
