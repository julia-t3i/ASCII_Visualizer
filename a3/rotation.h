#ifndef ROTATION_H
#define ROTATION_H

#include "model.h"

#define SCALE 30.0
#define DIST  5.0
#define WIDTH  80
#define HEIGHT 40
#define ROTATION_ANGLE ((2 * 3.14) / FRAME_COUNT)

ScreenVertex vertex_projection(float x, float y, float z);
ScreenVertex calculate_position(Vertex pt, int frame);
// void write_vertex(Model *m, int start, int end, int write_fd, int frame);

#endif
