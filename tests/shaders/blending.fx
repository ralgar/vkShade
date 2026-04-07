// blending.fx
// Tests all supported blend operations and write masks.
// Each cell in the 3x3 grid is an isolated test — red base, blue src, specific blend op.
// Green = pass, red = fail.

#include "tests_common.fxh"

// One render target per test
texture RT_Add      { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Alpha    { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Sub      { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_RevSub   { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Min      { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Max      { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Mask     { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_SepAlpha { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_NoBlend  { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };

sampler S_Add      { Texture = RT_Add; };
sampler S_Alpha    { Texture = RT_Alpha; };
sampler S_Sub      { Texture = RT_Sub; };
sampler S_RevSub   { Texture = RT_RevSub; };
sampler S_Min      { Texture = RT_Min; };
sampler S_Max      { Texture = RT_Max; };
sampler S_Mask     { Texture = RT_Mask; };
sampler S_SepAlpha { Texture = RT_SepAlpha; };
sampler S_NoBlend  { Texture = RT_NoBlend; };

static const uint ROWS = 3;
static const uint COLS = 3;

// Source colors used as base (dest) and src in blend operations
float4 PS_Red(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return float4(COLOR_RED, 0.5);
}

float4 PS_Blue(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return float4(COLOR_BLUE, 0.5);
}

// Final pass: Reads each RT, compares against expected value, then renders pass/fail grid.
float4 PS_Grid(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float2 local_uv;

    // (0,0) Additive: ONE*blue + ONE*red = (1.0, 0.0, 1.0)
    if (cell_uv(uv, 0, 0, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_Add, uv).rgb, float3(COLOR_MAGENTA));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (1,0) Alpha blend: lerp(red, blue, 0.5) = (0.5, 0.0, 0.5)
    if (cell_uv(uv, 1, 0, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_Alpha, uv).rgb, float3(COLOR_MAGENTA * 0.5));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (2,0) Subtract: blue - red = (0.0, 0.0, 1.0) clamped
    if (cell_uv(uv, 2, 0, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_Sub, uv).rgb, float3(COLOR_BLUE));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (0,1) Rev subtract: red - blue = (1.0, 0.0, 0.0) clamped
    if (cell_uv(uv, 0, 1, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_RevSub, uv).rgb, float3(COLOR_RED));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (1,1) Min: min(red, blue) = (0.0, 0.0, 0.0)
    if (cell_uv(uv, 1, 1, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_Min, uv).rgb, float3(COLOR_BLACK));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (2,1) Max: max(red, blue) = (1.0, 0.0, 1.0)
    if (cell_uv(uv, 2, 1, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_Max, uv).rgb, float3(COLOR_MAGENTA));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (0,2) Write mask R only: blue.R=0 overwrites red.R=1, GB preserved = (0.0, 0.0, 0.0)
    if (cell_uv(uv, 0, 2, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_Mask, uv).rgb, float3(COLOR_BLACK));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (1,2) Separate alpha op: color ADD = (1.0, 0.0, 1.0), alpha MAX(0.5, 0.5) = 0.5
    if (cell_uv(uv, 1, 2, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal4(tex2D(S_SepAlpha, uv), float4(COLOR_MAGENTA, 0.5));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (2,2) No blend: plain overwrite = (0.0, 0.0, 1.0)
    if (cell_uv(uv, 2, 2, COLS, ROWS, local_uv))
    {
        bool passed = approx_equal3(tex2D(S_NoBlend, uv).rgb, float3(COLOR_BLUE));
        return float4(cell_result(local_uv, passed), 1.0);
    }

    return float4(COLOR_BACKGROUND, 1.0);
}

technique BlendTest
{
    // (0,0) Additive
    pass Add_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Add;
        ClearRenderTargets = true;
    }

    pass Add_Blend
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Add;
        BlendEnable = true;
        SrcBlend = ONE;
        DestBlend = ONE;
        BlendOp = ADD;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = ONE;
        BlendOpAlpha = ADD;
    }

    // (1,0) Alpha blend
    pass Alpha_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Alpha;
        ClearRenderTargets = true;
    }

    pass Alpha_Blend
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Alpha;
        BlendEnable = true;
        SrcBlend = SRCALPHA;
        DestBlend = INVSRCALPHA;
        BlendOp = ADD;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = INVSRCALPHA;
        BlendOpAlpha = ADD;
    }

    // (2,0) Subtract
    pass Sub_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Sub;
        ClearRenderTargets = true;
    }

    pass Sub_Blend
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Sub;
        BlendEnable = true;
        SrcBlend = ONE;
        DestBlend = ONE;
        BlendOp = SUBTRACT;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = ONE;
        BlendOpAlpha = SUBTRACT;
    }

    // (0,1) Reverse subtract
    pass RevSub_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_RevSub;
        ClearRenderTargets = true;
    }

    pass RevSub_Blend
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_RevSub;
        BlendEnable = true;
        SrcBlend = ONE;
        DestBlend = ONE;
        BlendOp = REVSUBTRACT;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = ONE;
        BlendOpAlpha = REVSUBTRACT;
    }

    // (1,1) Min
    pass Min_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Min;
        ClearRenderTargets = true;
    }

    pass Min_Blend
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Min;
        BlendEnable = true;
        SrcBlend = ONE;
        DestBlend = ONE;
        BlendOp = MIN;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = ONE;
        BlendOpAlpha = MIN;
    }

    // (2,1) Max
    pass Max_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Max;
        ClearRenderTargets = true;
    }

    pass Max_Blend
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Max;
        BlendEnable = true;
        SrcBlend = ONE;
        DestBlend = ONE;
        BlendOp = MAX;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = ONE;
        BlendOpAlpha = MAX;
    }

    // (0,2) Write mask R only
    pass Mask_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Mask;
        ClearRenderTargets = true;
    }

    pass Mask_Write
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Mask;
        RenderTargetWriteMask = 1;
    }

    // (1,2) Separate alpha op
    pass SepAlpha_Base
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_SepAlpha;
        ClearRenderTargets = true;
    }

    pass SepAlpha_Blend
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_SepAlpha;
        BlendEnable = true;
        SrcBlend = ONE;
        DestBlend = ONE;
        BlendOp = ADD;
        SrcBlendAlpha = ONE;
        DestBlendAlpha = ONE;
        BlendOpAlpha = MAX;
    }

    // (2,2) No blend
    pass NoBlend_Write
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_NoBlend;
        ClearRenderTargets = true;
    }

    // Composite
    pass Grid
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Grid;
    }
}
