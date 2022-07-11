#version 300 es
precision highp float;
precision highp int;

// This shader draws the editing handle for simulation objects.

// Whether to draw the handle with a hole in the middle
uniform bool hole;
// If the handle is selected
uniform bool selected;

out vec4 color;

void main() {
    vec2 pos = 2.0 * gl_PointCoord - vec2(1.0, 1.0);
    // max single coordinate distance from center
    float rect_dist = max(abs(pos.x), abs(pos.y));
    if(rect_dist >= 0.551 || !hole) {
        color = selected ? vec4(1.0, 0.5, 0.15, 1.0) : vec4(0.7, 0.7, 0.7, 1.0);
    } else {
        discard;
    }
}