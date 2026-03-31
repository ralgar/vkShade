#include "ReShade.fxh"

uniform int RandomPos <
    source = "random";
    min = 0;
    max = 100;
> = 0;

float4 RandomTestPS(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // Bind the back buffer so we don't crash
    // FIXME: This probably needs to be fixed in the layer?
    float4 color = tex2D(ReShade::BackBuffer, uv);

    float barPos = RandomPos / 100.0;
    float bar = abs(uv.x - barPos) < 0.01 ? 1.0 : 0.0;
    return float4(0.0, bar, 0.0, 1.0);
}

technique RandomTest
{
    pass
    {
        VertexShader = PostProcessVS;
        PixelShader  = RandomTestPS;
    }
}
