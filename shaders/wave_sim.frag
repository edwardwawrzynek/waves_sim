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

// Get the value of the wave at a point. Point is the point to get, and u_neighbor is the value of the neighbor of point being considered. u_neighbor is returned if the point is a boundary.
float get_value(ivec2 point, float u_neighbor) {
    ivec2 tex_size = textureSize(sim_texture, 0);
    // if point is outside simulation area, fix wave value to u_neighbor.
    // this creates the boundary condition u_x = 0
    if(point.x < 0 || point.x >= tex_size.x || point.y < 0 || point.y >= tex_size.y) {
        return u_neighbor;
    }

    // sample point
    vec4 point_val = texelFetch(sim_texture, point, 0);
    // if the alpha channel is non zero, this is a boundary with condition u_x = 0
    if(point_val.a != 0.0) {
        return u_neighbor;
    }
    // otherwise, we can use the sample point
    return point_val.x;
}

// calculate the new u_tt value for a point based on its neighbors
float calc_wave_eq(ivec2 point, float u_point) {
    // get neighbors and calculate laplacian (via second symmetric derivative)
    float u0 = get_value(ivec2(point.x - 1, point.y), u_point);
    float u1 = get_value(ivec2(point.x + 1, point.y), u_point);
    float u2 = get_value(ivec2(point.x, point.y - 1), u_point);
    float u3 = get_value(ivec2(point.x, point.y + 1), u_point);

    float laplace = (u0 + u1 + u2 + u3 - 4.0 * u_point) / (delta_x * delta_x);

    return wave_c * wave_c * laplace;
}

void main() {
    if(gl_FragCoord.x >= 511.0 && gl_FragCoord.x < 512.0 && gl_FragCoord.y >= 511.0 && gl_FragCoord.y < 512.0 && time < 2.5133) {
        color = vec4(8.0 * exp(-0.5 * time) * sin(time * 5.0), 8.0 * (-0.5 * exp(-0.5 * time) * sin(5.0 * time) + 5.0 * exp(-0.5 * time) * cos(5.0 * time)), 0.0, 0.0);
    } else {
        vec4 point = texelFetch(sim_texture, ivec2(gl_FragCoord.xy), 0);
        float u = point.x;
        float u_t = point.y;

        float u_tt = calc_wave_eq(ivec2(gl_FragCoord.xy), u);

        u_t += u_tt * delta_t;
        u += u_t * delta_t;

        color = vec4(u, u_t, point.b, point.a);
    }
}