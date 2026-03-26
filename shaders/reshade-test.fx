#include "ReShade.fxh"

float4 PS_Main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float3 color = tex2D(ReShade::BackBuffer, uv).rgb;

    float2 pixel = uv * float2(BUFFER_WIDTH, BUFFER_HEIGHT);

    float gridSize = 64.0;
    float lineWidth = 1.0;

    float2 modPixel = pixel % gridSize;
    float line = step(modPixel.x, lineWidth) + step(modPixel.y, lineWidth);
    line = clamp(line, 0.0, 1.0);

    // Cyan instead of magenta to distinguish from GLSL version
    float3 gridColor = float3(0.0, 1.0, 1.0);

    color = lerp(color, gridColor, line);

    return float4(color, 1.0);
}

technique Passthrough
{
    pass
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Main;
    }
}
