#version 460
layout (local_size_x=1, local_size_y=1) in;
layout (rgba32f, binding = 0) uniform image2D h1;
layout (rgba32f, binding = 1) uniform image2D h2;

/*
 * This shader needs to be called after compute.glsl.
 * It's task is to update the old texture to hold new values
 * It is required to have memory barrier between the shaders.
 * r <- h_new
 * b <- h_n
 * 
 */

uniform float padding;

void main() {
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy) + ivec2(padding, padding);
    vec4 h = imageLoad(h2, pixel_coords);
    imageStore(h1, pixel_coords, vec4(h.r,h.g,0.0,1.0));
}
