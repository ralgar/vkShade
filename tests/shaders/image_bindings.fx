#include "ReShade.fxh"

// Images are available in the `reshade-shaders-git` AUR package
texture Tex0 < source = "LensDB.png"; >;
texture Tex1 < source = "LensDB2.png"; >;
texture Tex2 < source = "LensDOV.png"; >;
texture Tex3 < source = "LensDUV.png"; >;

sampler S0 { Texture = Tex0; };
sampler S1 { Texture = Tex1; };
sampler S2 { Texture = Tex2; };
sampler S3 { Texture = Tex3; };

float4 PS_MultiBinding(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // Bind the back buffer so we don't crash
    // FIXME: This probably needs to be fixed in the layer?
    float4 color = tex2D(ReShade::BackBuffer, uv);

    // Scale UVs to sample the full texture in each quadrant
    float2 scaledUV = frac(uv * 2.0);

    float4 result;
    if (uv.x < 0.5 && uv.y < 0.5)
        result = tex2D(S0, scaledUV);  // Top-left
    else if (uv.x >= 0.5 && uv.y < 0.5)
        result = tex2D(S1, scaledUV);  // Top-right
    else if (uv.x < 0.5 && uv.y >= 0.5)
        result = tex2D(S2, scaledUV);  // Bottom-left
    else
        result = tex2D(S3, scaledUV);  // Bottom-right

    return result;
}

technique MultiBinding
{
    pass
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_MultiBinding;
    }
}
