#version 300 es
precision highp float;
precision highp int;

layout(location=0) out vec4 color;

uniform sampler2D sim_texture;

// hello world
vec2 cmul(vec2 a, vec2 b) {
  return vec2(a.x * b.x - a.y * b.y, a.x * b.y + b.x * a.y);
}

vec2 csquare(vec2 c) {
  return cmul(c, c);
}

void main() {
    vec2 screen_pos = gl_FragCoord.xy / vec2(640.0, 480.0); // get screen pos in [0, 1]

    vec2 c = screen_pos * vec2(2.5, 2.0) - vec2(1.8, 1.0);
    vec2 z = vec2(0.0);

    color = vec4(0.0, 0.0, 0.0, 1.0);

    for(int i = 0; i < 200; i++) {
        z = csquare(z) + c;
        if(length(z) > 2.0) {
            float es = pow(float(i) / 200.0, 0.5);
            color = vec4(es * vec3(1.0, 0.5, 0.0), 1.0);
            break;
        }
    }

    color = color * 0.9 + 0.1 * texture(sim_texture, screen_pos.xy);
}