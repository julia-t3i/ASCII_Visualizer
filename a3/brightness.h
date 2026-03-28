#ifndef BRIGHTNESS_H
#define BRIGHTNESS_H

// from Hadeel's progress, take from her header file later
typedef struct {
    float x;
    float y;
    float z;
} Vertex;

typedef struct {
    int v1;
    int v2;
    int v3;
} Face;

typedef struct {
    Vertex *vertices;
    int num_vertices;
    int vertex_capacity;

    Face *faces;
    int num_faces;
    int face_capacity;
} Model;


// light direction, constant for all faces (should be normalized to unit length, we can pre-normalize it since it's constant)
// light currently points diagonally downwards towards the viewer

#define LIGHT_X  0.0f
#define LIGHT_Y  0.7071f
#define LIGHT_Z  0.7071f

float face_brightness(const Model *m, const Face *f);
char brightness_to_char(float b);
void write_ascii(const Model *m, int start, int end, int write_fd);

#endif