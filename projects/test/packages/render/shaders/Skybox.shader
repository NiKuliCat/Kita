Shader "Kita/Environment/Skybox"
{
    Properties
    {
        _Albedo("Skybox", TextureCube) = "black"
    }

    Pass "Skybox"
    {
        LightMode = PostProcess

        RenderState
        {
            Cull = Off
            DepthTest = LessEqual
            DepthWrite = Off
            Blend = Off
        }

        Program
        {
            Source = "Skybox.slang"
            Vertex = VSMain
            Fragment = PSMain
        }
    }
}
