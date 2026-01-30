//================================================================================
// Fullscreen Vertex Shader
//--------------------------------------------------------------------------------
// - Draws a single "full‐screen triangle" (no vertex buffer needed).
// - Passes a 2D UV to the fragment shader for NDC -> UV reconstructions.
//================================================================================
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
    // Full‐screen triangle trick. We index into a small array of clip‐space positions:
    vec2 pos[3] = vec2[](
        vec2(-1.0, -1.0),  // Bottom‐left
        vec2( 3.0, -1.0),  // Bottom‐right (x > 1 pushes beyond clip)
        vec2(-1.0,  3.0)   // Top‐left (y > 1 pushes beyond clip)
    );

    // gl_VertexIndex is 0,1,2 for the three vertices
    gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);

    // Remap clip‐space (–1..+1) -> UV (0..1)
    vUV = pos[gl_VertexIndex] * 0.5 + 0.5;
}
