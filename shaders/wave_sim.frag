#version 300 es
precision highp float;
precision highp int;

// Simulation state is stored in rgba float texture. Red is position (u), green is velocity (u_t), blue is inverse index of refraction, alpha marks boundaries.
layout(location=0) out vec4 color;
uniform sampler2D sim_texture;

// Distance between center of each texel (in m).
uniform float delta_x;
// Size of each time step (in s)
uniform float delta_t;
// Wave speed in free space (in m/s)
uniform float wave_speed_vacuum;

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
float calc_wave_eq(ivec2 point, float u_point, float wave_speed) {
    // get neighbors and calculate laplacian (via second symmetric derivative)
    float u0 = get_value(ivec2(point.x - 1, point.y), u_point);
    float u1 = get_value(ivec2(point.x + 1, point.y), u_point);
    float u2 = get_value(ivec2(point.x, point.y - 1), u_point);
    float u3 = get_value(ivec2(point.x, point.y + 1), u_point);

    float laplace = (u0 + u1 + u2 + u3 - 4.0 * u_point) / (delta_x * delta_x);

    return wave_speed * wave_speed * laplace;
}

// return damping factor for a point (in pixel coordinates)
// damping is used near edges to try to absorb waves, rather than reflect them
float damping_area_size = 0.15;
float damping(vec2 point) {
    // normalize point coordinates to be within [0, 1]
    point = point / vec2(textureSize(sim_texture, 0));

    // get distance from point to closest edge
    float dist = min(min(point.x, point.y), min(1.0 - point.x, 1.0 - point.y));

    if(dist < damping_area_size) {
        return 0.1 * (dist / damping_area_size) + 0.9;
    } else {
        return 1.0;
    }
}

void main() {
    vec4 point = texelFetch(sim_texture, ivec2(gl_FragCoord.xy), 0);
    float u = point.x;
    float u_t = point.y;
    float ior_inv = point.z;

    float u_tt = calc_wave_eq(ivec2(gl_FragCoord.xy), u, ior_inv * wave_speed_vacuum);

    u_t += u_tt * delta_t;
    u_t *= damping(gl_FragCoord.xy);

    u += u_t * delta_t;

    color = vec4(u, u_t, point.b, point.a);
}