// mip_test.fx - a row of squares in the top left corner, green = pass.
// Left to right:
//  0  R16F 256x256, every level
//  1  RGBA8 256x256, every level
//  2  Non-square 512x128, every level
//  3  Non-power-of-two 300x100, smallest level (approximate)
//  4  Partial chain (MipLevels = 4)
//  5  Multiple render targets
//  6  GenerateMipmaps = false
//  7  Never written texture is zero at every level
//  8  History: previous frame's mips are readable (first frame fails)
//  9  File texture with mips (upload path), needs mip_test_half.png

uniform int FrameCount < source = "framecount"; >;

texture BackBufferTex : COLOR;
sampler BackBuffer { Texture = BackBufferTex; };

#define POINT_SAMPLER(name, tex) \
    sampler name { Texture = tex; MinFilter = POINT; MagFilter = POINT; MipFilter = POINT; }

texture R16Tex     { Width = 256; Height = 256; Format = R16F;  MipLevels = 9; };
texture Rgba8Tex   { Width = 256; Height = 256; Format = RGBA8; MipLevels = 9; };
texture WideTex    { Width = 512; Height = 128; Format = RG16F; MipLevels = 10; };
texture NpotTex    { Width = 300; Height = 100; Format = R16F;  MipLevels = 9; };
texture PartialTex { Width = 256; Height = 256; Format = R16F;  MipLevels = 4; };
texture MrtATex    { Width = 256; Height = 256; Format = R16F;  MipLevels = 9; };
texture MrtBTex    { Width = 256; Height = 256; Format = R16F;  MipLevels = 9; };
texture NoGenTex   { Width = 256; Height = 256; Format = R16F;  MipLevels = 9; };
texture BlankTex   { Width = 256; Height = 256; Format = RGBA8; MipLevels = 9; };
texture HistTex    { Width = 256; Height = 256; Format = R16F;  MipLevels = 9; };
texture PrevTex    { Width = 1;   Height = 1;   Format = R16F; };
texture FileTex < source = "mip_test_half.png"; > { Width = 256; Height = 256; Format = RGBA8; MipLevels = 9; };

POINT_SAMPLER(R16, R16Tex);
POINT_SAMPLER(Rgba8, Rgba8Tex);
POINT_SAMPLER(Wide, WideTex);
POINT_SAMPLER(Npot, NpotTex);
POINT_SAMPLER(Partial, PartialTex);
POINT_SAMPLER(MrtA, MrtATex);
POINT_SAMPLER(MrtB, MrtBTex);
POINT_SAMPLER(NoGen, NoGenTex);
POINT_SAMPLER(Blank, BlankTex);
POINT_SAMPLER(Hist, HistTex);
POINT_SAMPLER(Prev, PrevTex);
POINT_SAMPLER(File, FileTex);

void FullscreenVS(uint id : SV_VertexID, out float4 pos : SV_Position, out float2 uv : TEXCOORD)
{
    uv = float2((id << 1) & 2, id & 2);
    pos = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}

// Changes every frame, so stale mips are caught
float Value(int frame) { return (frame % 2 == 0) ? 0.25 : 0.75; }

// Ramps give every texel of every level a known value: its centre coordinate
float RampPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return uv.x * Value(FrameCount);
}

float4 ColorRampPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return float4(uv.x, uv.y, 1.0 - uv.x, Value(FrameCount));
}

float2 WidePS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return uv;
}

float XPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return uv.x;
}

void MrtPS(float4 pos : SV_Position, float2 uv : TEXCOORD,
           out float a : SV_Target0, out float b : SV_Target1)
{
    a = uv.x;
    b = uv.y;
}

float OnePS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target { return 1.0; }

float ValuePS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return Value(FrameCount);
}

// Runs before HistTex is rewritten, so this reads the previous frame's smallest mip
float PrevPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return tex2Dlod(Hist, float4(0.5, 0.5, 0, 8)).x;
}

bool Near(float a, float b, float tolerance) { return abs(a - b) <= tolerance; }

// Centre of the texel just past the middle, for a dimension of 'size' texels
float Centre(int size) { return (floor(size * 0.5) + 0.5) / size; }

bool Test(int index)
{
    bool ok = true;

    if (index == 0 || index == 1 || index == 4 || index == 5)
    {
        for (int level = 0; level < 9; level++)
        {
            int size = 256 >> level;
            float c = Centre(size);
            float4 at = float4(c, c, 0, level);

            if (index == 0)
                ok = ok && Near(tex2Dlod(R16, at).x, c * Value(FrameCount), 0.005);
            if (index == 1)
            {
                float4 expected = float4(c, c, 1.0 - c, Value(FrameCount));
                float4 got = tex2Dlod(Rgba8, at);
                ok = ok && Near(got.r, expected.r, 0.02) && Near(got.g, expected.g, 0.02)
                        && Near(got.b, expected.b, 0.02) && Near(got.a, expected.a, 0.02);
            }
            if (index == 4 && level < 4)
                ok = ok && Near(tex2Dlod(Partial, at).x, c, 0.005);
            if (index == 5)
                ok = ok && Near(tex2Dlod(MrtA, at).x, c, 0.005)
                        && Near(tex2Dlod(MrtB, at).x, c, 0.005);
        }
    }
    else if (index == 2)
    {
        for (int level = 0; level < 10; level++)
        {
            float x = Centre(max(1, 512 >> level));
            float y = Centre(max(1, 128 >> level));
            float2 got = tex2Dlod(Wide, float4(x, y, 0, level)).xy;
            ok = ok && Near(got.x, x, 0.005) && Near(got.y, y, 0.005);
        }
    }
    else if (index == 3)
    {
        ok = Near(tex2Dlod(Npot, float4(0.5, 0.5, 0, 8)).x, 0.5, 0.05);
    }
    else if (index == 6)
    {
        ok = tex2Dlod(NoGen, float4(0.5, 0.5, 0, 0)).x > 0.99
          && tex2Dlod(NoGen, float4(0.5, 0.5, 0, 8)).x < 0.01;
    }
    else if (index == 7)
    {
        for (int level = 0; level < 9; level++)
        {
            float4 got = tex2Dlod(Blank, float4(0.5, 0.5, 0, level));
            ok = ok && (got.r + got.g + got.b + got.a) == 0.0;
        }
    }
    else if (index == 8)
    {
        ok = Near(tex2Dlod(Prev, float4(0.5, 0.5, 0, 0)).x, Value(FrameCount + 1), 0.005);
    }
    else if (index == 9)
    {
        ok = Near(tex2Dlod(File, float4(0.25, 0.5, 0, 1)).x, 0.0, 0.01)
          && Near(tex2Dlod(File, float4(0.75, 0.5, 0, 1)).x, 1.0, 0.01)
          && Near(tex2Dlod(File, float4(0.5, 0.5, 0, 8)).x, 0.5, 0.01);
    }

    return ok;
}

float4 ShowPS(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // 40 pixel squares with 10 pixel gaps
    int index = int((pos.x - 20.0) / 50.0);
    if (pos.x >= 20.0 && index < 10 && (pos.x - 20.0) - index * 50.0 < 40.0
        && pos.y >= 20.0 && pos.y < 60.0)
    {
        return Test(index) ? float4(0, 1, 0, 1) : float4(1, 0, 0, 1);
    }

    return tex2D(BackBuffer, uv);
}

#define PASS(ps) VertexShader = FullscreenVS; PixelShader = ps

technique MipTest
{
    pass { PASS(RampPS);      RenderTarget = R16Tex; }
    pass { PASS(ColorRampPS); RenderTarget = Rgba8Tex; }
    pass { PASS(WidePS);      RenderTarget = WideTex; }
    pass { PASS(XPS);         RenderTarget = NpotTex; }
    pass { PASS(XPS);         RenderTarget = PartialTex; }
    pass { PASS(MrtPS);       RenderTarget0 = MrtATex; RenderTarget1 = MrtBTex; }
    pass { PASS(OnePS);       RenderTarget = NoGenTex; GenerateMipmaps = false; }
    pass { PASS(PrevPS);      RenderTarget = PrevTex; }
    pass { PASS(ValuePS);     RenderTarget = HistTex; }
    pass { PASS(ShowPS); }
}
