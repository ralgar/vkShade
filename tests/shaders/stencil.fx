// stencil.fx
// Comprehensive stencil operation and comparison function tests.
// Each cell is isolated — no shared stencil state between tests.
// Green = pass, red = fail.

#include "tests_common.fxh"

static const uint ROWS = 3;
static const uint COLS = 5;

// One render target per test cell
texture RT_ReplaceEqual        { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_ReplaceNotEqual     { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Keep                { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Zero                { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_IncrClamp           { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_DecrClamp           { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_IncrWrap            { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_DecrWrap            { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Invert              { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Never               { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Always              { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Less                { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_Greater             { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_WriteMask           { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
texture RT_ReadMask            { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };

sampler S_ReplaceEqual        { Texture = RT_ReplaceEqual; };
sampler S_ReplaceNotEqual     { Texture = RT_ReplaceNotEqual; };
sampler S_Keep                { Texture = RT_Keep; };
sampler S_Zero                { Texture = RT_Zero; };
sampler S_IncrClamp           { Texture = RT_IncrClamp; };
sampler S_DecrClamp           { Texture = RT_DecrClamp; };
sampler S_IncrWrap            { Texture = RT_IncrWrap; };
sampler S_DecrWrap            { Texture = RT_DecrWrap; };
sampler S_Invert              { Texture = RT_Invert; };
sampler S_Never               { Texture = RT_Never; };
sampler S_Always              { Texture = RT_Always; };
sampler S_Less                { Texture = RT_Less; };
sampler S_Greater             { Texture = RT_Greater; };
sampler S_WriteMask           { Texture = RT_WriteMask; };
sampler S_ReadMask            { Texture = RT_ReadMask; };

// Color outputs
float4 PS_Red(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target   { return float4(COLOR_RED,   1.0); }
float4 PS_Green(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target { return float4(COLOR_GREEN, 1.0); }
float4 PS_Blue(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target  { return float4(COLOR_BLUE,  1.0); }
float4 PS_Black(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target { return float4(COLOR_BLACK, 1.0); }

float4 PS_Grid(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float2 local_uv;

    // (0,0) REPLACE + EQUAL: write ref=1, then pass only where stencil==1 — expect green
    if (cell_uv(uv, 0, 0, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_ReplaceEqual, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (1,0) REPLACE + NOT_EQUAL: write ref=1, then pass only where stencil!=1 — expect black (no pixels pass)
    if (cell_uv(uv, 1, 0, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_ReplaceNotEqual, uv).rgb == COLOR_BLACK);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (2,0) KEEP: write ref=1, keep op leaves stencil at 1, subsequent EQUAL pass — expect green
    if (cell_uv(uv, 2, 0, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_Keep, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (3,0) ZERO: write ref=1, zero op clears to 0, subsequent EQUAL ref=0 pass — expect green
    if (cell_uv(uv, 3, 0, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_Zero, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (4,0) INCREMENT_AND_CLAMP: write ref=255, increment clamps to 255, EQUAL ref=255 — expect green
    if (cell_uv(uv, 4, 0, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_IncrClamp, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (0,1) DECREMENT_AND_CLAMP: write ref=0, decrement clamps to 0, EQUAL ref=0 — expect green
    if (cell_uv(uv, 0, 1, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_DecrClamp, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (1,1) INCREMENT_AND_WRAP: write ref=255, increment wraps to 0, EQUAL ref=0 — expect green
    if (cell_uv(uv, 1, 1, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_IncrWrap, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (2,1) DECREMENT_AND_WRAP: write ref=0, decrement wraps to 255, EQUAL ref=255 — expect green
    if (cell_uv(uv, 2, 1, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_DecrWrap, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (3,1) INVERT: write ref=0xAA (170), invert gives 0x55 (85), EQUAL ref=85 — expect green
    if (cell_uv(uv, 3, 1, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_Invert, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (4,1) NEVER: stencil test always fails, nothing written — expect black
    if (cell_uv(uv, 4, 1, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_Never, uv).rgb == COLOR_BLACK);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (0,2) ALWAYS: stencil test always passes regardless of value — expect green
    if (cell_uv(uv, 0, 2, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_Always, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (1,2) LESS: write ref=5, test ref=6 < stencil=5 fails, test ref=4 < stencil=5 passes
    //   Two sub-passes: blue where ref=6 (fail), green where ref=4 (pass) — expect green
    if (cell_uv(uv, 1, 2, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_Less, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (2,2) GREATER: write ref=5, test ref=4 > stencil=5 fails, test ref=6 > stencil=5 passes — expect green
    if (cell_uv(uv, 2, 2, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_Greater, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (3,2) WRITE MASK: write ref=0xFF with mask=0x0F, only low nibble written (0x0F),
    //   subsequent EQUAL ref=0x0F passes — expect green
    if (cell_uv(uv, 3, 2, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_WriteMask, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    // (4,2) READ MASK: write ref=0xFF, test ref=0x0F with read mask=0x0F,
    //   masked stencil=0x0F == masked ref=0x0F — expect green
    if (cell_uv(uv, 4, 2, COLS, ROWS, local_uv))
    {
        bool passed = all(tex2D(S_ReadMask, uv).rgb == COLOR_GREEN);
        return float4(cell_result(local_uv, passed), 1.0);
    }

    return float4(COLOR_BACKGROUND, 1.0);
}

technique StencilTest
{
    // (0,0) REPLACE + EQUAL
    // Pass 1: write ref=1 to stencil via REPLACE
    pass ReplaceEqual_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_ReplaceEqual;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 1;
    }
    // Pass 2: draw green only where stencil==1
    pass ReplaceEqual_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_ReplaceEqual;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 1;
    }
    // Pass 3: rejection check — red must not appear where stencil==1
    pass ReplaceEqual_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_ReplaceEqual;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 1;
    }

    // (1,0) REPLACE + NOT_EQUAL
    // Pass 1: write ref=1 everywhere
    pass ReplaceNotEqual_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_ReplaceNotEqual;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 1;
    }
    // Pass 2: draw green only where stencil!=1 — no pixels should pass
    pass ReplaceNotEqual_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_ReplaceNotEqual;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 1;
    }

    // (2,0) KEEP
    // Pass 1: write ref=1 via REPLACE
    pass Keep_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Keep;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 1;
    }
    // Pass 2: KEEP leaves stencil at 1
    pass Keep_Keep
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Keep;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = ALWAYS;
        StencilRef = 1;
    }
    // Pass 3: verify stencil still 1 via EQUAL
    pass Keep_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_Keep;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 1;
    }
    // Pass 4: rejection check — red must not appear where stencil==1
    pass Keep_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Keep;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 1;
    }

    // (3,0) ZERO
    // Pass 1: write ref=1
    pass Zero_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Zero;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 1;
    }
    // Pass 2: ZERO clears stencil to 0
    pass Zero_Zero
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Zero;
        StencilEnable = true;
        StencilPass = ZERO;
        StencilFunc = ALWAYS;
        StencilRef = 0;
    }
    // Pass 3: verify stencil is 0 via EQUAL ref=0
    pass Zero_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_Zero;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 0;
    }
    // Pass 4: rejection check — red must not appear where stencil==0
    pass Zero_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Zero;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 0;
    }

    // (4,0) INCREMENT_AND_CLAMP
    // Pass 1: write ref=255
    pass IncrClamp_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_IncrClamp;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 255;
    }
    // Pass 2: increment clamps to 255
    pass IncrClamp_Incr
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_IncrClamp;
        StencilEnable = true;
        StencilPass = INCRSAT;
        StencilFunc = ALWAYS;
        StencilRef = 255;
    }
    // Pass 3: verify still 255
    pass IncrClamp_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_IncrClamp;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 255;
    }
    // Pass 4: rejection check — red must not appear where stencil==255
    pass IncrClamp_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_IncrClamp;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 255;
    }

    // (0,1) DECREMENT_AND_CLAMP
    // Pass 1: write ref=0
    pass DecrClamp_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_DecrClamp;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 0;
    }
    // Pass 2: decrement clamps to 0
    pass DecrClamp_Decr
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_DecrClamp;
        StencilEnable = true;
        StencilPass = DECRSAT;
        StencilFunc = ALWAYS;
        StencilRef = 0;
    }
    // Pass 3: verify still 0
    pass DecrClamp_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_DecrClamp;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 0;
    }
    // Pass 4: rejection check — red must not appear where stencil==0
    pass DecrClamp_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_DecrClamp;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 0;
    }

    // (1,1) INCREMENT_AND_WRAP
    // Pass 1: write ref=255
    pass IncrWrap_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_IncrWrap;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 255;
    }
    // Pass 2: increment wraps to 0
    pass IncrWrap_Incr
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_IncrWrap;
        StencilEnable = true;
        StencilPass = INCR;
        StencilFunc = ALWAYS;
        StencilRef = 255;
    }
    // Pass 3: verify wrapped to 0
    pass IncrWrap_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_IncrWrap;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 0;
    }
    // Pass 4: rejection check — red must not appear where stencil==0
    pass IncrWrap_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_IncrWrap;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 0;
    }

    // (2,1) DECREMENT_AND_WRAP
    // Pass 1: write ref=0
    pass DecrWrap_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_DecrWrap;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 0;
    }
    // Pass 2: decrement wraps to 255
    pass DecrWrap_Decr
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_DecrWrap;
        StencilEnable = true;
        StencilPass = DECR;
        StencilFunc = ALWAYS;
        StencilRef = 0;
    }
    // Pass 3: verify wrapped to 255
    pass DecrWrap_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_DecrWrap;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 255;
    }
    // Pass 4: rejection check — red must not appear where stencil==255
    pass DecrWrap_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_DecrWrap;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 255;
    }

    // (3,1) INVERT
    // Pass 1: write ref=0xAA (170)
    pass Invert_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Invert;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 170;
    }
    // Pass 2: invert gives 0x55 (85)
    pass Invert_Invert
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Invert;
        StencilEnable = true;
        StencilPass = INVERT;
        StencilFunc = ALWAYS;
        StencilRef = 85;
    }
    // Pass 3: verify stencil is now 85
    pass Invert_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_Invert;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 85;
    }
    // Pass 4: rejection check — red must not appear where stencil==85
    pass Invert_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Invert;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 85;
    }

    // (4,1) NEVER
    // Pass 1: clear to black
    pass Never_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Never;
        ClearRenderTargets = true;
    }
    // Pass 2: NEVER — nothing should be written
    pass Never_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_Never;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NEVER;
        StencilRef = 0;
    }

    // (0,2) ALWAYS
    // Pass 1: clear to black, write arbitrary stencil value
    pass Always_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Always;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 42;
    }
    // Pass 2: ALWAYS passes regardless of ref vs stencil
    pass Always_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_Always;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = ALWAYS;
        StencilRef = 0;
    }
    // No reject pass for ALWAYS — by definition it cannot be used to test rejection

    // (1,2) LESS: ref < stencil passes
    // Pass 1: write stencil=5
    pass Less_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Less;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 5;
    }
    // Pass 2: ref=6, 6 < stencil=5 fails — write blue
    pass Less_Fail
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Less;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = LESS;
        StencilRef = 6;
    }
    // Pass 3: ref=4, 4 < stencil=5 passes — write green
    pass Less_Pass
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_Less;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = LESS;
        StencilRef = 4;
    }
    // Pass 4: rejection check — ref=4, 4 < stencil=5 would pass, so use GREATER to reject
    pass Less_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Less;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = GREATER;
        StencilRef = 5;
    }

    // (2,2) GREATER: ref > stencil passes
    // Pass 1: write stencil=5
    pass Greater_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_Greater;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 5;
    }
    // Pass 2: ref=4, 4 > stencil=5 fails — write blue
    pass Greater_Fail
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Blue;
        RenderTarget = RT_Greater;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = GREATER;
        StencilRef = 4;
    }
    // Pass 3: ref=6, 6 > stencil=5 passes — write green
    pass Greater_Pass
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_Greater;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = GREATER;
        StencilRef = 6;
    }
    // Pass 4: rejection check — ref=6, 6 > stencil=5 would pass, so use LESS to reject
    pass Greater_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_Greater;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = LESS;
        StencilRef = 5;
    }

    // (3,2) WRITE MASK: only low nibble written
    // Pass 1: write ref=0xFF with write mask=0x0F — only 0x0F written
    pass WriteMask_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_WriteMask;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 255;
        StencilWriteMask = 15;
    }
    // Pass 2: verify stencil is 0x0F via EQUAL ref=15
    pass WriteMask_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_WriteMask;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 15;
    }
    // Pass 3: rejection check — red must not appear where stencil==15
    pass WriteMask_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_WriteMask;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 15;
    }

    // (4,2) READ MASK: only low nibble compared
    // Pass 1: write ref=0xFF
    pass ReadMask_Setup
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Black;
        RenderTarget = RT_ReadMask;
        ClearRenderTargets = true;
        StencilEnable = true;
        StencilPass = REPLACE;
        StencilFunc = ALWAYS;
        StencilRef = 255;
    }
    // Pass 2: ref=0x0F, read mask=0x0F — masked stencil=0x0F == masked ref=0x0F, passes
    pass ReadMask_Test
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Green;
        RenderTarget = RT_ReadMask;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = EQUAL;
        StencilRef = 15;
        StencilReadMask = 15;
    }
    // Pass 3: rejection check — ref=0, read mask=0x0F, masked stencil=0x0F != masked ref=0x00, rejects
    pass ReadMask_Reject
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Red;
        RenderTarget = RT_ReadMask;
        StencilEnable = true;
        StencilPass = KEEP;
        StencilFunc = NOTEQUAL;
        StencilRef = 15;
        StencilReadMask = 15;
    }

    // Composite
    pass Grid
    {
        VertexShader = PostProcessVS;
        PixelShader = PS_Grid;
    }
}
