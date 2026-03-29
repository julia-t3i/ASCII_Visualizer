#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

#include "model.h"

// light direction, constant for all faces (should be normalized to unit length, we can pre-normalize it since it's constant)
// light currently points diagonally downwards towards the viewer

#define LIGHT_X  0.5774f
#define LIGHT_Y  0.5774f
#define LIGHT_Z  0.5774f

float face_brightness(const Model *m, const Face *f);
char brightness_to_char(float b);
// void write_ascii(const Model *m, int start, int end, int write_fd);

#endif