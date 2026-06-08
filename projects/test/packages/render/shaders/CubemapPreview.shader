Material "Kita/Preview/CubemapPreview"
{
    Domain = Utility

    Properties
    {
        _BaseColor("Tint", Color) = (1.0, 1.0, 1.0, 1.0)
        _Albedo("Cubemap", TextureCube) = "black"
    }

    Pass "Preview"
    {
        LightMode = ForwardOpaque

        RenderState
        {
            Cull = Back
            DepthTest = LessEqual
            DepthWrite = On
            Blend = Off
        }

        Program
        {
            Source = "CubemapPreview.slang"
            Vertex = VSMain
            Fragment = PSMain
        }
    }
}
