float3 EvaluateToonLighting(
    KitaDeferredSurfaceData surface,
    KitaDeferredLightingContext context)
{
    float3 N = surface.NormalWS;
    float3 L = context.LightDirectionWS;
    float3 V = context.ViewDirectionWS;

    float threshold = saturate(surface.CustomData.x);
    float specBand = max(surface.CustomData.y, 0.001);

    float NdotL = saturate(dot(N, L));
    float lightStep = step(threshold, NdotL);
    float3 diffuse = surface.BaseColor * context.LightColor * context.LightIntensity * lightStep;

    float3 H = SafeNormalize(V + L, N);
    float specularStep = step(1.0 - specBand, saturate(dot(N, H)));
    float3 specular = context.LightColor * context.LightIntensity * specularStep * 0.25;

    float rim = pow(saturate(1.0 - dot(N, V)), 2.0);
    float3 rimLighting = surface.BaseColor * rim * 0.15;

    float3 indirect = EvaluateDiffuseIBL(N, surface.BaseColor, 0.0) * surface.AmbientOcclusion * 0.2;
    return diffuse + specular + indirect + rimLighting + surface.Emissive;
}
