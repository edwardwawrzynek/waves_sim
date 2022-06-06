#version 300 es
precision highp float;
precision highp int;

// Simulation state is stored in rgba float texture. Red is position (u), green is velocity (u_t), blue is wave speed (c).
layout(location=0) out vec4 color;
uniform sampler2D sim_texture;

// Distance between center of each texel (in m).
uniform float delta_x;
// Size of each time step (in s)
uniform float delta_t;

float wave_c = 2.0;

uniform float time;

// Get the value of the wave at a point
float get_value(ivec2 point) {
    ivec2 tex_size = textureSize(sim_texture, 0);
    // if point is outside simulation area, fix wave value to closest point in area.
    // This creates the boundry condition u_x = 0
    if(point.x < 0) {
        return texelFetch(sim_texture, ivec2(0, point.y), 0).x;
    } else if (point.x >= tex_size.x) {
        return texelFetch(sim_texture, ivec2(tex_size.x - 1, point.y), 0).x;
    } else if(point.y < 0) {
        return texelFetch(sim_texture, ivec2(point.x, 0), 0).x;
    } else if (point.y >= tex_size.y) {
        return texelFetch(sim_texture, ivec2(point.x, tex_size.y - 1), 0).x;
    }
    return texelFetch(sim_texture, point, 0).x;
}

// calculate the new u_tt value for a point based on its neighbors
float calc_wave_eq(ivec2 point, float u_point) {
    // get neighbors and calculate laplacian (via second symmetric derivative)
    float u0 = get_value(ivec2(point.x - 1, point.y));
    float u1 = get_value(ivec2(point.x + 1, point.y));
    float u2 = get_value(ivec2(point.x, point.y - 1));
    float u3 = get_value(ivec2(point.x, point.y + 1));

    float laplace = (u0 + u1 + u2 + u3 - 4.0 * u_point) / (delta_x * delta_x);

    return wave_c * wave_c * laplace;
}

void main() {
    if(gl_FragCoord.x >= 127.0 && gl_FragCoord.x < 128.0 && gl_FragCoord.y >= 127.0 && gl_FragCoord.y < 128.0) {
        color = vec4(1.0 * sin(time * 5.0), 0.0, 0.0, 1.0);
    } else {
        vec4 point = texelFetch(sim_texture, ivec2(gl_FragCoord.xy), 0);
        float u = point.x;
        float u_t = point.y;

        float u_tt = calc_wave_eq(ivec2(gl_FragCoord.xy), u);

        u_t += u_tt * delta_t;
        u += u_t * delta_t;

        color = vec4(u, u_t, 0.0, 1.0);
    }
}