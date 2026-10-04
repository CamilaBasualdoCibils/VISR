#version 450 core

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec2 uv;

void main() {
    gl_Position = inPosition;
    uv = inUV;
}
