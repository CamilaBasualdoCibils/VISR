#version 450 core

layout(binding = 0) uniform sampler2D glyphBitmap;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

void main() {
    float coverage = texture(glyphBitmap, uv).r;
    outColor = vec4(1.0, 1.0, 1.0, coverage);
}
