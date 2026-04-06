// clear.fx
// Tests ClearRenderTargets behavior.
// (0,0): Writes red, then clears and additively blends blue. Expected: blue (red discarded).
// (1,0): Writes red, then additively blends blue without clearing. Expected: magenta (red preserved).

#include "tests_common.fxh"

texture RT_Clear   { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_NoClear { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };

sampler S_Clear   { Texture = RT_Clear; };
sampler S_NoClear { Texture = RT_NoClear; };

static const uint ROWS = 1;
static const uint COLS = 2;

float4 PS_Grid(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float2 local_uv;

    // (0,0) Clear: Red written, then cleared before additive blue. Expect: blue
    if (cell_uv(uv, 0, 0, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_Clear, uv).rgb, COLOR_BLUE);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (1,0) No clear: Red written, blue additively blended on top. Expect: magenta
    if (cell_uv(uv, 1, 0, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_NoClear, uv).rgb, COLOR_MAGENTA);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    return float4(COLOR_BACKGROUND, 1.0);
}

technique ClearTest
{
    // (0,0) Write red, then clear and additively blend blue
    pass Clear_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Clear;
        ClearRenderTargets = true;
    }

    pass Clear_Write
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Clear;
        ClearRenderTargets = true;
        BlendEnable = true;
        SrcBlend = ONE;
        DestBlend = ONE;
        BlendOp = ADD;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = ONE;
        BlendOpAlpha = ADD;
    }

    // (1,0) Write red, additively blend blue without clearing
    pass NoClear_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_NoClear;
        ClearRenderTargets = true;
    }

    pass NoClear_Blend
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_NoClear;
        ClearRenderTargets = false;
        BlendEnable = true;
        SrcBlend = ONE;
        DestBlend = ONE;
        BlendOp = ADD;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = ONE;
        BlendOpAlpha = ADD;
    }

    // Composite
    pass Grid
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Grid;
    }
}
