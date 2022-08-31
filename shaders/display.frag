#version 300 es
precision highp float;
precision highp int;

out vec4 color;

uniform sampler2D sim_texture;
// screen size of window on which we are displaying (in pixels)
uniform vec2 screen_size;
// size of absorbing boundary layer in sim_texture (in texels)
uniform float damping_area_size;

void main() {
    vec2 damping_offset = vec2(damping_area_size, damping_area_size);
    vec2 tex_size = vec2(textureSize(sim_texture, 0));
    vec2 damping_relative_cover = damping_offset / tex_size;

    // get screen pos in [0, 1]
    vec2 screen_pos = gl_FragCoord.xy / screen_size;
    // get position in texture
    vec2 sim_pos = screen_pos * (vec2(1.0, 1.0) - 2.0 * damping_relative_cover) + damping_relative_cover;
    vec4 point = texture(sim_texture, sim_pos);
    // draw boundaries white
    if(point.a > 0.0) {
        color = vec4(1.0, 1.0, 1.0, 1.0);
    }
    // otherwise color based on wave value
    else if(point.x > 0.0) {
        color = vec4(point.x, 0.0, 0.0, 1.0);
    } else {
        color = vec4(0.0, 0.0, -point.x, 1.0);
    }
    // lightly highlight areas with non-1 index of refraction in green
    if(point.b < 1.0) {
        color.g = 0.5 - 0.5 * point.b;
    }
}