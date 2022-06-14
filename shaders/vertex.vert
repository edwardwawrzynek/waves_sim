#version 300 es
precision highp float;
precision highp int;

layout (location = 0) in vec2 vertex_pos;
uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(vertex_pos, 0.0, 1.0);
}