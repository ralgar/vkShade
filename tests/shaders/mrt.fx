#include "ReShade.fxh"

// 8 intermediate render targets
texture RT0 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT1 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT2 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT3 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT4 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT5 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT6 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT7 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };

sampler SRT0 { Texture = RT0; };
sampler SRT1 { Texture = RT1; };
sampler SRT2 { Texture = RT2; };
sampler SRT3 { Texture = RT3; };
sampler SRT4 { Texture = RT4; };
sampler SRT5 { Texture = RT5; };
sampler SRT6 { Texture = RT6; };
sampler SRT7 { Texture = RT7; };

// Pass 0: Write a distinct solid color to each of the 8 render targets
void PS_WriteTargets(float4 pos : SV_Position, float2 uv : TEXCOORD,
    out float4 o0 : SV_Target0,
    out float4 o1 : SV_Target1,
    out float4 o2 : SV_Target2,
    out float4 o3 : SV_Target3,
    out float4 o4 : SV_Target4,
    out float4 o5 : SV_Target5,
    out float4 o6 : SV_Target6,
    out float4 o7 : SV_Target7)
{
    o0 = float4(1, 0, 0, 1);    // Red
    o1 = float4(0, 1, 0, 1);    // Green
    o2 = float4(0, 0, 1, 1);    // Blue
    o3 = float4(1, 1, 0, 1);    // Yellow
    o4 = float4(0, 1, 1, 1);    // Cyan
    o5 = float4(1, 0, 1, 1);    // Magenta
    o6 = float4(1, 0.5, 0, 1);  // Orange
    o7 = float4(1, 1, 1, 1);    // White
}

// Pass 1: Sample all 8 render targets and display them in an 8-cell grid
float4 PS_DisplayTargets(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float2 scaled = uv * float2(4, 2);
    int col = (int)scaled.x;
    int row = (int)scaled.y;
    int cell = row * 4 + col;
    float2 cellUV = frac(scaled);

    if      (cell == 0) return tex2D(SRT0, cellUV);
    else if (cell == 1) return tex2D(SRT1, cellUV);
    else if (cell == 2) return tex2D(SRT2, cellUV);
    else if (cell == 3) return tex2D(SRT3, cellUV);
    else if (cell == 4) return tex2D(SRT4, cellUV);
    else if (cell == 5) return tex2D(SRT5, cellUV);
    else if (cell == 6) return tex2D(SRT6, cellUV);
    else                return tex2D(SRT7, cellUV);
}

technique MRTTest
{
    pass WriteTargets
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_WriteTargets;
        RenderTarget0 = RT0;
        RenderTarget1 = RT1;
        RenderTarget2 = RT2;
        RenderTarget3 = RT3;
        RenderTarget4 = RT4;
        RenderTarget5 = RT5;
        RenderTarget6 = RT6;
        RenderTarget7 = RT7;
    }

    pass DisplayTargets
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_DisplayTargets;
    }
}
