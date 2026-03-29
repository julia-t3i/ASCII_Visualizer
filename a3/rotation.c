#include <unistd.h>
#include <stdlib.h>
#include <math.h>

#include "model.h"
#include "rotation.h"

/*
* Helper function which projects a 3D vertex to 2D vertex, which uses the screen 
* size and macros to scale.
*/
ScreenVertex vertex_projection(float x, float y, float z) {
    ScreenVertex sv;
    
    sv.x = (x / (z + DIST)) * SCALE + WIDTH / 2;
    sv.y = HEIGHT / 2 - (y / (z + DIST)) * SCALE;

    return sv;
}

/*
* Takes in a starting Vertex with coordinates (x, y, z) and calculates its new
* 2D position in (x, y) coordinates for the given frame.
*
*   pt: the regular Vertex to convert to a ScreenVertex
*   frame: the frame for which to calculate the rotation angle
*/
ScreenVertex calculate_position(Vertex pt, int frame) {
    float frame_angle = frame * ROTATION_ANGLE;

    float new_x = pt.x * cos(frame_angle) + pt.z * sin(frame_angle);
    float new_y = pt.y;
    float new_z = -pt.x * sin(frame_angle) + pt.z * cos(frame_angle);

    return vertex_projection(new_x, new_y, new_z);
}

// /*
// * Given a model m, and a range of vertices to write, and a fd to write to
// * write all Screen Vertices to pipe. 
// *
// *   m: the model supplying the vertices to convert
// *   start: the start index for vertices to convert
// *   end: the end index for vertices to convert
// *   write_fd: the file descriptor to write all ScreenVertex to
// *   frame: the frame number for which to calculate vertex positions
// *
// */
// void write_vertex(Model *m, int start, int end, int write_fd, int frame) {
//     for (int i = start; i < end; i++) {
//         ScreenVertex sv = calculate_position(m->vertices[i], frame);
//         ssize_t write_result = write(write_fd, &sv, sizeof(ScreenVertex));
//         if (write_result == -1) {
//             perror("write vertex");
//             exit(1);
//         } else if (write_result != sizeof(ScreenVertex)) {
//             perror("partial vertex write"); 
//             exit(1);
//         }
//     }
// }

