#version 300 es
precision highp float;
precision highp int;

// This shader draws objects to the simulation texture. Depending on which channels are enabled by
// glColorMask, it will do the following:
// * red/green: set the value and derivative of a point
// * blue: set the texel to be a medium with the given inverse index of refraction
// * alpha: set the texel to be a boundary
//
// Only one of the channel combinations above should be enabled at a time.

out vec4 color;

// Color to write: (u, u_t, inv_ior, boundary)
uniform vec4 object_props;

void main() {
    color = object_props;
}