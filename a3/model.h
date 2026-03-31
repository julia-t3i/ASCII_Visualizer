#ifndef MODEL_H
#define MODEL_H

#define MIN_CHILDREN 3
#define MAX_CHILDREN 70
#define TARGET_ITEMS_PER_CHILD 3000
#define FRAME_COUNT 72
#define FRAME_RATE 50000
#define CHUNK_SIZE 1800

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

typedef struct {
    /*
    * Contains info about the task being executed.
    * - frame: the frame for which is being calculated
    * - start: the first face index to be processed for this taskk
    * - end: the last face index to be processed for this task
    */
    int frame;
    int start;
    int end;
} TaskMsg;

typedef struct {
    /*
    * Contains info about a single child worker, which only exists in the parent.
    * - pid: the child's pid
    * - to_fd: the fd which the parent process writes info to
    * - from_fd: the fc which the child process writes info to
    * - busy: the child's status, 1 = busy, 0 = free
    */
    pid_t pid;
    int to_fd;
    int from_fd;
    int busy;
} Worker;

typedef struct {
    /*
    * Contains info about whether a child process is done, 
    * since parents need to ensure this before reading computed data.
    * - done: 1 = child computations are complete
    */
    int done;
} EndMsg;

Model *load_obj(const char *filename);
void free_model(Model *m);

#endif