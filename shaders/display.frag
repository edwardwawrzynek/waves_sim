#version 300 es
precision highp float;
precision highp int;

out vec4 color;

uniform sampler2D sim_texture;
uniform vec2 screen_size;

void main() {
    // get screen pos in [0, 1]
    vec2 screen_pos = gl_FragCoord.xy / screen_size;
    vec4 point = texture(sim_texture, screen_pos.xy);
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
}