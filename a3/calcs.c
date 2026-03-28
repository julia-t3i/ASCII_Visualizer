#include <math.h>
#include <unistd.h>
#include <stdlib.h>

#include "model.h"
#include "rotation.h"
#include "brightness.h"

/*
*   Given a model m, a range of faces, and a fd to write to,
*   write all Screen Vertices to pipe. 
*
*   m: the model supplying the vertices to convert
*   start: the start index for vertices to convert
*   end: the end index for vertices to convert
*   write_fd: the file descriptor to write all ScreenVertex to
*   frame: the frame number for which to calculate vertex positions
*
*   Implementation note:
*   - for each Face in m, calculate its brightness and the associated ASCII char  
*   - each of the Face's vertices are then projected & stored in ScreenVertex
*   - the ScreenVertex for a single face will share the same brightness
* 
*/

void write_screen_vertices(const Model *m, int start, int end, int write_fd, int frame)
 {
    for (int i = start; i < end; i++) {
        const Face *f = &m->faces[i];
        float brightness = face_brightness(m, f);
        char c = brightness_to_char(brightness);

        int face_verts[3] = {f->v1, f->v2, f->v3};
        for (int j = 0; j < 3; j++) {
            Vertex v = m->vertices[face_verts[j]];
            ScreenVertex sv = calculate_position(v, frame);
            sv.c = c;
            
            ssize_t write_result = write(write_fd, &sv, sizeof(ScreenVertex));
            if (write_result == -1) {
                perror("write vertex");
                exit(1);
            } else if (write_result != sizeof(ScreenVertex)) {
                perror("partial vertex write"); 
                exit(1);
            }
        }
    }
}
