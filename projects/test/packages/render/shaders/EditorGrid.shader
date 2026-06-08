Material "Kita/Editor/Grid"
{
    Domain = PostProcess

    Pass "Grid"
    {
        LightMode = PostProcess

        RenderState
        {
            Cull = Off
            DepthTest = LessEqual
            DepthWrite = Off
            Blend = On
        }

        Program
        {
            Source = "EditorGrid.slang"
            Vertex = VSMain
            Fragment = PSMain
        }
    }
}
