#include "ReShade.fxh"

// Time-based uniforms
uniform float  FrameTime  < source = "frametime"; >;
uniform int    FrameCount < source = "framecount"; >;
uniform float4 Date       < source = "date"; >;
uniform float  Timer      < source = "timer"; >;

// Animated uniforms
uniform float2 PingPong < source = "pingpong"; min = 0.0; max = 1.0; step = 0.5; >;
uniform int    Random   < source = "random"; min = 0; max = 100; >;

// Input uniforms (stubbed, just check they exist and don't crash)
uniform bool   Key        < source = "key"; keycode = 32; >;
uniform bool   MouseBtn   < source = "mousebutton"; keycode = 0; >;
uniform float2 MousePoint < source = "mousepoint"; >;
uniform float2 MouseDelta < source = "mousedelta"; >;
uniform float2 MouseWheel < source = "mousewheel"; >;

// State uniforms (stubbed)
uniform bool HasDepth       < source = "bufready_depth"; >;
uniform bool OverlayOpen    < source = "overlay_open"; >;
uniform int  OverlayActive  < source = "overlay_active"; >;
uniform int  OverlayHovered < source = "overlay_hovered"; >;
uniform bool Screenshot     < source = "screenshot"; >;

float4 SmokeTestPS(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // Bind the back buffer so we don't crash
    // FIXME: This probably needs to be fixed in the layer?
    float4 color = tex2D(ReShade::BackBuffer, uv);

    // All of these should be true if uniforms are working correctly
    bool frametimeOk  = FrameTime > 0.0;
    bool framecountOk = FrameCount >= 0;
    bool dateOk       = Date.x >= 2024.0 && Date.y >= 1.0 && Date.y <= 12.0
                        && Date.z >= 1.0 && Date.z <= 31.0 && Date.w >= 0.0;
    bool timerOk      = Timer >= 0.0;
    bool pingpongOk   = PingPong.x >= 0.0 && PingPong.x <= 1.0;
    bool randomOk     = Random >= 0 && Random <= 100;

    // Stubbed uniforms: Just check they don't produce garbage values
    bool mousepointOk = MousePoint.x >= 0.0 && MousePoint.y >= 0.0;
    bool overlayOk    = OverlayActive >= 0 && OverlayHovered >= 0;

    // Show test passes/fails color-coded columns across the screen
    // Green = passing, red = failed
    float x = uv.x;
    if      (x < 0.125) return frametimeOk  ? float4(0,1,0,1) : float4(1,0,0,1);
    else if (x < 0.250) return framecountOk ? float4(0,1,0,1) : float4(1,0,0,1);
    else if (x < 0.375) return dateOk       ? float4(0,1,0,1) : float4(1,0,0,1);
    else if (x < 0.500) return timerOk      ? float4(0,1,0,1) : float4(1,0,0,1);
    else if (x < 0.625) return pingpongOk   ? float4(0,1,0,1) : float4(1,0,0,1);
    else if (x < 0.750) return randomOk     ? float4(0,1,0,1) : float4(1,0,0,1);
    else if (x < 0.875) return mousepointOk ? float4(0,1,0,1) : float4(1,0,0,1);
    else                return overlayOk    ? float4(0,1,0,1) : float4(1,0,0,1);
}

technique UniformSmokeTest
{
    pass
    {
        VertexShader = PostProcessVS;
        PixelShader  = SmokeTestPS;
    }
}
