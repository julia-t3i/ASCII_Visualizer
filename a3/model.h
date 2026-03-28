#ifndef MODEL_H
#define MODEL_H


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


Model *load_obj(const char *filename);
void free_model(Model *m);

#endif