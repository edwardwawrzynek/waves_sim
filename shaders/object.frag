#version 300 es
precision highp float;
precision highp int;

// This shader draws objects to the simulation texture. Depending on which channels are enabled by
// glColorMask, it will do the following:
// * red/green: none (TODO)
// * blue: set the texel to be a medium with a given index of refraction
// * alpha: set the texel to be a boundary
//
// Only one of the channel combinations above should be enabled at a time.

out vec4 color;

// Inverse index of refraction to write
uniform float ior_inv;

// Boundary value texel to write
uniform float boundary_value;

void main() {
    color = vec4(0.0, 0.0, ior_inv, boundary_value);
}