#version 300 es
precision highp float;
precision highp int;

out vec4 color;

uniform sampler2D sim_texture;

void main() {
    // get screen pos in [0, 1]
    vec2 screen_pos = gl_FragCoord.xy / vec2(640.0, 480.0);
    color = texture(sim_texture, screen_pos.xy);
    color = vec4(color.rg, screen_pos.x, 1.0);
}