Shader "Kita/Editor/Picking"
{
    Pass "Picking"
    {
        LightMode = EditorPicking

        RenderState
        {
            Cull = Off
            DepthTest = Less
            DepthWrite = On
            Blend = Off
        }

        Program
        {
            Source = "EditorPicking.slang"
            Vertex = VSMain
            Fragment = PSMain
        }
    }
}
