#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "model.h"
#include "rotation.h"
#include "brightness.h"

static void draw_line(ScreenVertex a, ScreenVertex b, char c, int write_fd) {
    int x0 = (int)a.x;
    int y0 = (int)a.y;
    int x1 = (int)b.x;
    int y1 = (int)b.y;

    int dx = abs(x1 - x0); // total horizontal distance
    int dy = abs(y1 - y0); // total vertical distance
    int sx = x0 < x1 ? 1 : -1; // whether to step right or left
    int sy = y0 < y1 ? 1 : -1; // whether to step down or up
    int err = dx - dy; // error term, tracks how far off we are from the ideal line

    while (1) {
        // write this pixel
        if (x0 >= 0 && x0 < WIDTH && y0 >= 0 && y0 < HEIGHT) {
            ScreenVertex sv = {0};
            sv.x = x0;
            sv.y = y0;
            sv.c = c;
            ssize_t write_result = write(write_fd, &sv, sizeof(ScreenVertex));
            if (write_result == -1) {
                perror("write vertex");
                exit(1);
            }
        }

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

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
        // back-face culling — skip faces pointing away from viewer
        float brightness = face_brightness(m, f);
        if (brightness <= 0.0f) continue;

        char c = brightness_to_char(brightness);

        ScreenVertex sv0 = calculate_position(m->vertices[f->v1], frame);
        ScreenVertex sv1 = calculate_position(m->vertices[f->v2], frame);
        ScreenVertex sv2 = calculate_position(m->vertices[f->v3], frame);

        draw_line(sv0, sv1, c, write_fd);
        draw_line(sv1, sv2, c, write_fd);
        draw_line(sv2, sv0, c, write_fd);
    }
}
