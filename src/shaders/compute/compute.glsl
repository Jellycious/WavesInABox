#version 460

#define MAX_SOURCES 10

layout (local_size_x=1, local_size_y=1) in;
layout (rgba32f, binding = 0) uniform image2D h1;
layout (rgba32f, binding = 1) uniform image2D h2;
layout (rgba32f, binding = 2) uniform image2D normals;

uniform float delta;
uniform float csqrd;
uniform float time;
uniform float freq;
uniform float amplitude;
uniform float padding;
uniform float damping;

uniform ivec2 sources[MAX_SOURCES];
uniform float source_phases[MAX_SOURCES];
uniform float source_amplitude[MAX_SOURCES];
uniform float source_freq[MAX_SOURCES];

uniform ivec2 SIM_SIZE;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy) + ivec2(padding, padding);
    vec2 uv = vec2(float(pixel_coords.x) / gl_NumWorkGroups.x, float(pixel_coords.y) / gl_NumWorkGroups.y);

    // Get convolutional values
    vec4 h = imageLoad(h1, pixel_coords);
    vec4 h_yp = imageLoad(h1, pixel_coords + ivec2(0,1));
    vec4 h_yn = imageLoad(h1, pixel_coords + ivec2(0,-1));
    vec4 h_xp = imageLoad(h1, pixel_coords + ivec2(1,0));
    vec4 h_xn = imageLoad(h1, pixel_coords + ivec2(-1,0));

    // Apply discretization of 2D wave equation.
    float diff_x = h_xp.r - 2 * h.r + h_xn.r;
    float diff_y = h_yp.r - 2 * h.r + h_yn.r;
    float delta_sqrd = delta;
    float diff_sum = csqrd * delta_sqrd * (diff_x + diff_y);
    float h_new = 2 * h.r - h.g + diff_sum;

    // Apply Source Wave
    bool source = false;
    float source_phase = 0;
    float source_amp;
    float source_fq;
    for (int i = 0; i < MAX_SOURCES; i++) {
        bool source_is_active = (pixel_coords.x == sources[i].x + padding && pixel_coords.y == sources[i].y + padding);
        source  = source || source_is_active;
        if (source_is_active) {
            source_phase = source_phases[i];
            source_amp = source_amplitude[i];
            source_fq = source_freq[i];
        }
    }

    h_new = h_new + csqrd * delta * int(source) * sin(source_phase * source_fq) * source_amp;

    // Damping
    h_new -= damping * delta * (h_new - h.r);

    // output result in texture 2
    imageStore(h2, pixel_coords, vec4(h_new, h.r, h.g, 1.0));
}
