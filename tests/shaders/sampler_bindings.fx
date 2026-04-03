#include "ReShade.fxh"

texture Tex0 < source = "LensDB.png"; >;

sampler S0 { Texture = Tex0; AddressU = WRAP;   AddressV = WRAP;   };
sampler S1 { Texture = Tex0; AddressU = CLAMP;  AddressV = CLAMP;  };
sampler S2 { Texture = Tex0; AddressU = MIRROR; AddressV = MIRROR; };
sampler S3 { Texture = Tex0; AddressU = BORDER; AddressV = BORDER; };

float4 PS_SamplerTest(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // Bind the back buffer so we don't crash
    // FIXME: This probably needs to be fixed in the layer?
    float4 color = tex2D(ReShade::BackBuffer, uv);

    // Sample outside [0,1] so address modes are obvious
    float2 scaledUV = frac(uv * 2.0) * 3.0 - 1.0;

    float border = 0.02;
    float2 quadUV = frac(uv * 2.0);
    bool isBorder = any(quadUV < border) || any(quadUV > 1.0 - border);

    float4 borderColor;
    float4 result;

    if (uv.x < 0.5 && uv.y < 0.5)
    {
        borderColor = float4(1, 0, 0, 1);
        result = tex2D(S0, scaledUV);  // WRAP
    }
    else if (uv.x >= 0.5 && uv.y < 0.5)
    {
        borderColor = float4(0, 1, 0, 1);
        result = tex2D(S1, scaledUV);  // CLAMP
    }
    else if (uv.x < 0.5 && uv.y >= 0.5)
    {
        borderColor = float4(0, 0, 1, 1);
        result = tex2D(S2, scaledUV);  // MIRROR
    }
    else
    {
        borderColor = float4(1, 1, 0, 1);
        result = tex2D(S3, scaledUV);  // BORDER
    }

    return isBorder ? borderColor : result;
}

technique SamplerTest
{
    pass
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_SamplerTest;
    }
}
