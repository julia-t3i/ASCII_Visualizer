#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "model.h"
#include "brightness.h"

// characters ordered dark → bright
static const char *ASCII_CHARS = " .:-=+*#@";
static const int ASCII_CHARS_LEN = 9;  // strlen of ASCII_CHARS

/*
* Computes the brightness of a face based on its angle relative to the light
* direction. Uses the cross product to find the face normal, then takes the
* dot product with the light direction to get brightness.
* Returns a brightness value in [0, 1], where 0 is dark and 1 is bright.
*/
float face_brightness(const Model *m, const Face *f) {
    // get the vertices of the face
    Vertex v1 = m->vertices[f->v1];
    Vertex v2 = m->vertices[f->v2];
    Vertex v3 = m->vertices[f->v3];

    // compute the normal vector of the face using the cross product 
    float edge1_x = v2.x - v1.x;
    float edge1_y = v2.y - v1.y;
    float edge1_z = v2.z - v1.z;

    float edge2_x = v3.x - v1.x;
    float edge2_y = v3.y - v1.y;
    float edge2_z = v3.z - v1.z;

    float normal_x = edge1_y * edge2_z - edge1_z * edge2_y;
    float normal_y = edge1_z * edge2_x - edge1_x * edge2_z;
    float normal_z = edge1_x * edge2_y - edge1_y * edge2_x;

    // get unit vector of the normal
    float length = sqrtf(normal_x * normal_x + normal_y * normal_y + normal_z * normal_z);
    normal_x /= length;
    normal_y /= length;
    normal_z /= length;

    // compute the dot product with the light direction
    float brightness = normal_x * LIGHT_X + normal_y * LIGHT_Y + normal_z * LIGHT_Z;

    // clamp brightness to [0, 1]
    if (brightness < 0) {
        brightness = 0;
    }
    return brightness;
}

/*
* Maps a brightness value to an ASCII character representing that brightness.
* Uses a ramp of characters ordered from dark to bright.
*
*   b: brightness value in [0, 1]
*
* Returns the ASCII character corresponding to the brightness.
*
*/
char brightness_to_char(float b) {
    // map brightness [0, 1] to an index in ASCII_CHARS
    int index = (int)(b * (ASCII_CHARS_LEN - 1) + 0.5f); // round to nearest index since casting floors towards zero
    if (index < 0) {
        index = 0;
    } else if (index >= ASCII_CHARS_LEN) {
        index = ASCII_CHARS_LEN - 1;
    }
    return ASCII_CHARS[index];
}


// /*
// * Computes the brightness of each face in the given range [start, end) and writes
// * the corresponding ASCII character to the pipe.
// *
// *   m: the model containing the face and vertex data
// *   start: the start index of faces to process
// *   end: the end index of faces to process (exclusive)
// *   write_fd: the file descriptor to write ASCII characters to
// *
// */
// void write_ascii(const Model *m, int start, int end, int write_fd, int frame) {
//     for (int i = start; i < end; i++) {
//         float brightness = face_brightness(m, &m->faces[i]);
//         char c = brightness_to_char(brightness);
//         if (write(write_fd, &c, sizeof(char)) == -1) {
//             perror("write ASCII character to pipe");
//             exit(1);
//         } 
//     }
// }