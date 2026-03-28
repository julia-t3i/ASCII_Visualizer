#ifndef MODEL_H
#define MODEL_H

#define MIN_CHILDREN 3
#define MAX_CHILDREN 10
#define TARGET_ITEMS_PER_CHILD 3000
#define FRAME_COUNT 72
#define FRAME_RATE 50000

typedef struct {
    /*
    * Stores info about a 3D point.
    *   float x: original x coordinate
    *   float y: original y coordinate
    *   float z: original z coordinate
    */
    float x;
    float y;
    float z;
} Vertex;

typedef struct {
    /*
    * Stores info about a 2D point. 
    *   float x: transformed x coordinate
    *   float y: transformed y coordinate
    */
    float x;
    float y;
    char c;

} ScreenVertex;

typedef struct {
    /*
    * Stores vertices of a single Face
    */
    int v1;
    int v2;
    int v3;
} Face;

typedef struct {
    /*
    * Stores info about an object Model 
    */
    Vertex *vertices;
    int num_vertices;
    int vertex_capacity;

    Face *faces;
    int num_faces;
    int face_capacity;
} Model;

typedef struct {
    /*
    * Dictates beginning and end face indices for child
    * processes to operate on.
    */
    int start;
    int end;
} Range;

Model *load_obj(const char *filename);
void free_model(Model *m);

#endif