#version 300 es
precision highp float;
precision highp int;

layout (location = 0) in vec2 vPosition;

void main() {
    gl_Position = vec4(vPosition, 1.0, 1.0);
}