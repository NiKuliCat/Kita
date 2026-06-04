Shader "Kita/PostProcess/ToneMapping"
{
    Pass "ToneMapping"
    {
        LightMode = PostProcess

        RenderState
        {
            Cull = Off
            DepthTest = Off
            DepthWrite = Off
            Blend = Off
        }

        Program
        {
            Source = "ToneMapping.slang"
            Vertex = VSMain
            Fragment = PSMain
        }
    }
}
