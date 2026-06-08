Material "Kita/Deferred/Lighting"
{
    Domain = PostProcess

    Pass "DeferredLighting"
    {
        LightMode = DeferredLighting

        RenderState
        {
            Cull = Off
            DepthTest = Always
            DepthWrite = On
            Blend = Off
        }

        Program
        {
            Source = "DeferredLighting.slang"
            Vertex = VSMain
            Fragment = PSMain
        }
    }
}
