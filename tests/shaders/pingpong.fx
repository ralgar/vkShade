#include "ReShade.fxh"

uniform float PingPongValue <
    source = "pingpong";
    min = 0.0;
    max = 1.0;
    step = 0.5;
    smoothing = 0.1;
> = 0.0;

float4 PingPongTestPS(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // Bind the back buffer so we don't crash
    // FIXME: This probably needs to be fixed in the layer?
    float4 color = tex2D(ReShade::BackBuffer, uv);

    return float4(lerp(float3(1, 0, 0), float3(0, 0, 1), PingPongValue), 1.0);
}

technique PingPongTest
{
    pass
    {
        VertexShader = PostProcessVS;
        PixelShader  = PingPongTestPS;
    }
}
