#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D uInput;

void main()
{
    vec3 col = texture(uInput, vUV).rgb;
    float gray = dot(col, vec3(0.299, 0.587, 0.114));
    fragColor = vec4(vec3(gray), 1.0);
}
