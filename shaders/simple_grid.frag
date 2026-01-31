#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D uInput;

void main()
{
    // Sample the input image
    vec3 color = texture(uInput, vUV).rgb;

    // Convert UV to pixel coordinates
    ivec2 texSize = textureSize(uInput, 0);
    vec2 pixel = vUV * vec2(texSize);

    // Grid parameters
    float gridSize = 64.0;    // Pixels per cell
    float lineWidth = 1.0;    // Line width in pixels

    // Compute distance to nearest grid line
    vec2 modPixel = mod(pixel, gridSize);
    float line = step(modPixel.x, lineWidth) + step(modPixel.y, lineWidth);
    line = clamp(line, 0.0, 1.0);

    // Grid color
    vec3 gridColor = vec3(1.0, 0.0, 1.0);

    // Blend grid over original image
    color = mix(color, gridColor, line);

    fragColor = vec4(color, 1.0);
}
