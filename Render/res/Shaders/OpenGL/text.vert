#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec2 uv;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    uv = inUV;
}
