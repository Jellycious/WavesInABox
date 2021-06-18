#include <cglm/cglm.h>

#ifndef UTIL_H
#define UTIL_H

bool generatePlane(int N, int size, float* ptr, unsigned int* indices);

void util_scale(mat4 m, float s);
void util_translation(mat4 m, float x, float y, float z);
#endif
