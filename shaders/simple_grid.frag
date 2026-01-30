#version 450

layout(location = 0) in  vec4 uv;
layout(location = 0) out vec4 outColor;

void main()
{
    float grid = step(0.95, fract(uv.x * 20.0)) + step(0.95, fract(uv.y * 20.0));
    outColor = vec4(vec3(grid), 1.0);
}
