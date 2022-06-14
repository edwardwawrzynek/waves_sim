#version 300 es
precision highp float;
precision highp int;

// This shader draws objects to the simulation texture. Depending on which channels are enabled by
// glColorMask, it will do the following:
// * red/green: none (TODO)
// * blue: set the texel to be a medium with a given wave speed
// * alpha: set the texel to be a boundary
//
// Only one of the channel combinations above should be enabled at a time.

out vec4 color;

// Wave speed to write
uniform float wave_speed;

void main() {
    color = vec4(0.0, 0.0, wave_speed, 1.0);
}