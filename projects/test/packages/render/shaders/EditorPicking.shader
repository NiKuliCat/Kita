Material "Kita/Editor/Picking"
{
    Domain = Utility

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
