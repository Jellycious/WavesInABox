#include <stdio.h>

#include "util.hpp"

bool generatePlane(int N, int size, float* vertices, unsigned int* indices) {
    /*
     * N must be bigger than 1.
     * vertices must be an array of 5*N*N floats
     *      The vertices will be packed in he following manner
     *      (x,y,z) (u,v), where xyz are the position and uv are the texture coordinate.
     * indices must be an array of 3*2*(N-1)*(N-1) ints
     */
    if (N < 2) {
        printf("Can't generate mesh, N should be > 1\n");
        return false; 
    }

    double wd = size / (double) (N-1);
    double zd = size / (double) (N-1);
    double wtd = 1.0 / (double) (N-1);
    double ztd = 1.0 / (double) (N-1);

    // vertices are stores column first.
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            vertices[j*5+N*i*5] = ((float)-size / 2.0) + i * wd;
            vertices[j*5+N*i*5+1] = 0.0f; 
            vertices[j*5+N*i*5+2] = ((float) -size / 2.0) + j * zd;
            vertices[j*5+N*i*5+3] = wtd * i;
            vertices[j*5+N*i*5+4] = 1.0 - ztd * j;
        }
    }

    for (int i = 0; i < (N-1); i++) {
        for (int j = 0; j < (N-1); j++) {
            unsigned int tl = j+N*i;
            unsigned int tr = j+N*(i+1);
            unsigned int bl = (j+1)+N*i;
            unsigned int br = (j+1)+N*(i+1);
            indices[6*j + (N-1) * 6 * i] = tl;
            indices[6*j + (N-1) * 6 * i + 1] = tr;
            indices[6*j + (N-1) * 6 * i + 2] = bl;
            indices[6*j + (N-1) * 6 * i + 3] = bl;
            indices[6*j + (N-1) * 6 * i + 4] = tr;
            indices[6*j + (N-1) * 6 * i + 5] = br;
        }
    }

    return true;
}

// Matrix transformations
void util_translation(mat4 m, float x, float y, float z) {
    float v[3] = {x, y, z};
    glm_translate(m, v);
}

void util_scale(mat4 m, float s) {
    float sv[3] = {s,s,s};
    glm_scale(m, sv);
}


