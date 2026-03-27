#include <math.h>
#include <unistd.h>

#define FRAME_COUNT 72
#define SCALE 30.0
#define DIST  5.0
#define WIDTH  80
#define HEIGHT 40
#define ROTATION_ANGLE ((2 * M_PI) / FRAME_COUNT)

typedef struct {
    /*
    * Stores info about a 2D point. 
    *   float xc: original x coordinate
    *   float yc: original y coordinate
    */
    float x;
    float y;

} ScreenVertex;

typedef struct {
    /*
    * Stores info about a 3D point.
    *   float xc: original x coordinate
    *   float yc: original y coordinate
    *   float zc: original z coordinate
    */
    float x;
    float y;
    float z;

} Vertex;

typedef struct {
    /*
    * Stores info about a single Face
    */
    int v1;
    int v2;
    int v3;
} Face;

typedef struct {
    /*
    * Stores info about an object Model, 
    */

    Vertex *vertices;
    int num_vertices;
    int vertex_capacity;

    Face *faces;
    int num_faces;
    int face_capacity;
} Model;

/*
* Helper function which projects a 3D vertex to 2D vertex, which uses the screen 
* size and macros to scale.
*/
ScreenVertex vertex_projection(float x, float y, float z) {
    ScreenVertex sv;
    
    sv.x = (x / (z + DIST)) * SCALE + WIDTH / 2;
    sv.y = (y / (z + DIST)) * SCALE + HEIGHT / 2;

    return sv;
}

/*
* Takes in a starting Point with coordinates (x, y, z) and calculates its new
* 2D position in (x, y) coordinates for the given frame.
*
*   pt: the regular Vertex to convert to a ScreenVertex
*   frame: the frame for which to calculate the rotation angle
*/
ScreenVertex calculate_position(Vertex pt, float frame) {
    float frame_angle = frame * ROTATION_ANGLE;

    float new_x = pt.x * cos(frame_angle) + pt.z * sin(frame_angle);
    float new_y = pt.y;
    float new_z = -pt.x * sin(frame_angle) + pt.z * cos(frame_angle);

    return vertex_projection(new_x, new_y, new_z);
}


/*
* Given a model m, and a range of vertices to write, and a fd to write to
* write all Screen Vertices to pipe. 
*
*   m: the object supplying the vertices to convert
*   start: the start index for vertices to convert
*   end: the end index for vertices to convert
*   write_fd: the file descriptor to write all ScreenVertex to
*   frame: the frame number for which to calculate vertex positions
*
*/
void write_vertex(Model *m, int start, int end, int write_fd, int frame) {
    for (int i = start; i < end; i++) {
        ScreenVertex sv = calculate_position(m->vertices[i], frame);
        ssize_t write_result = write(write_fd, &sv, sizeof(ScreenVertex);
        if (write_result == -1) {
            perror("write vertex");
            exit(1);
        } else if (write_result != sizeof(ScreenVertex)) {
            perror("partial vertex write"); 
            exit(1);
        }
    }
}

