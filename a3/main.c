#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>

#include "model.h"
#include "calcs.h"
#include "rotation.h"
#include "brightness.h"

/*
* Initializes an empty Model object for the given .obj file.
*/
static void init_model(Model *m) {
    if (m == NULL) {
        fprintf(stderr, "Error: init_model received NULL model pointer.\n");
        exit(1);
    }

    m->num_vertices = 0;
    m->vertex_capacity = 1000;
    m->vertices = malloc(sizeof(Vertex) * m->vertex_capacity);
    if (m->vertices == NULL) {
        perror("malloc");
        exit(1);
    }

    m->num_faces = 0;
    m->face_capacity = 1000;
    m->faces = malloc(sizeof(Face) * m->face_capacity);
    if (m->faces == NULL) {
        perror("malloc");
        free(m->vertices);
        exit(1);
    }
}

/*
* Helper which adds the given Vertex to the given Model.
*/
static void add_vertex(Model *m, float x, float y, float z) {
    Vertex *tmp;

    if (m == NULL) {
        fprintf(stderr, "Error: add_vertex received NULL model pointer.\n");
        exit(1);
    }

    if (m->num_vertices >= m->vertex_capacity) {
        m->vertex_capacity *= 2;
        tmp = realloc(m->vertices, sizeof(Vertex) * m->vertex_capacity);
        if (tmp == NULL) {
            perror("realloc");
            free(m->vertices);
            free(m->faces);
            free(m);
            exit(1);
        }
        m->vertices = tmp;
    }

    m->vertices[m->num_vertices].x = x;
    m->vertices[m->num_vertices].y = y;
    m->vertices[m->num_vertices].z = z;
    m->num_vertices++;
}

/*
* Helper which adds the given Face to the given Model.
*/
static void add_face(Model *m, int a, int b, int c) {
    Face *tmp;

    if (m == NULL) {
        fprintf(stderr, "Error: add_face received NULL model pointer.\n");
        exit(1);
    }

    if (m->num_faces >= m->face_capacity) {
        m->face_capacity *= 2;
        tmp = realloc(m->faces, sizeof(Face) * m->face_capacity);
        if (tmp == NULL) {
            perror("realloc");
            free(m->vertices);
            free(m->faces);
            free(m);
            exit(1);
        }
        m->faces = tmp;
    }

    m->faces[m->num_faces].v1 = a;
    m->faces[m->num_faces].v2 = b;
    m->faces[m->num_faces].v3 = c;
    m->num_faces++;
}

/*
* Parses the line containing a vertex's coordinates into a Vertex object,
* then adds it to the given Model.
*/
static void parse_vertex_line(Model *m, char *line) {
    float x, y, z;

    if (m == NULL || line == NULL) {
        fprintf(stderr, "Error: parse_vertex_line received NULL argument.\n");
        exit(1);
    }

    if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3) {
        add_vertex(m, x, y, z);
    } else {
        fprintf(stderr, "Error: invalid vertex line: %s", line);
        exit(1);
    }
}

/* 
* Parses the line containing a face's edges into a Face object,
* then adds it to the given Model.
*
*   Preconditions:
*   - parsed lines must represent 4-edge faces
*/
static void parse_face_line(Model *m, char *line) {
    int a, b, c, d;

    if (m == NULL || line == NULL) {
        fprintf(stderr, "Error: parse_face_line received NULL argument.\n");
        exit(1);
    }

    if (sscanf(line, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d",
               &a, &b, &c, &d) == 4) {
        add_face(m, a - 1, b - 1, c - 1);
        add_face(m, a - 1, c - 1, d - 1);
    }

    else if (sscanf(line, "f %d//%*d %d//%*d %d//%*d %d//%*d",
                    &a, &b, &c, &d) == 4) {
        add_face(m, a - 1, b - 1, c - 1);
        add_face(m, a - 1, c - 1, d - 1);
    }

    else if (sscanf(line, "f %d/%*d %d/%*d %d/%*d %d/%*d",
                    &a, &b, &c, &d) == 4) {
        add_face(m, a - 1, b - 1, c - 1);
        add_face(m, a - 1, c - 1, d - 1);
    }

    else if (sscanf(line, "f %d %d %d %d", &a, &b, &c, &d) == 4) {
        add_face(m, a - 1, b - 1, c - 1);
        add_face(m, a - 1, c - 1, d - 1);
    }
    else {
        fprintf(stderr, "Error: only quad faces are supported.");
        exit(1);
    }
}

/*
* Initializes and populates a Model object containing all vertices and faces 
* for a given .obj file.
*/
Model *load_obj(const char *filename) {
    FILE *fp;
    Model *m;
    char line[1024];

    if (filename == NULL) {
        fprintf(stderr, "Error: load_obj received NULL filename.\n");
        return NULL;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        perror("fopen");
        return NULL;
    }

    m = malloc(sizeof(Model));
    if (m == NULL) {
        perror("malloc");
        fclose(fp);
        return NULL;
    }

    init_model(m);

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == 'v' && line[1] == ' ') {
            parse_vertex_line(m, line);
        } else if (line[0] == 'f' && line[1] == ' ') {
            parse_face_line(m, line);
        }
    }

    if (ferror(fp)) {
        perror("fgets");
        fclose(fp);
        free(m->vertices);
        free(m->faces);
        free(m);
        return NULL;
    }

    if (fclose(fp) != 0) {
        perror("fclose");
        free(m->vertices);
        free(m->faces);
        free(m);
        return NULL;
    }

    return m;
}

/*
* Adjusts the children clamp based on defined macros for max & min children.
*/
static int clamp(int x, int low, int high) {
    if (x < low) return low;
    if (x > high) return high;
    return x;
}

static Range make_range(int i, int k, int n) {
    Range r;

    if (k <= 0) {
        fprintf(stderr, "Error: make_range received invalid k = %d.\n", k);
        exit(1);
    }

    if (i < 0 || i >= k) {
        fprintf(stderr, "Error: make_range received invalid i = %d for k = %d.\n", i, k);
        exit(1);
    }

    if (n < 0) {
        fprintf(stderr, "Error: make_range received negative n = %d.\n", n);
        exit(1);
    }

    r.start = (i * n) / k;
    r.end = ((i + 1) * n) / k;
    return r;
}

static void choose_child_counts(const Model *m, int *num_children) {
    int total_children;

    if (m == NULL || num_children == NULL) {
        fprintf(stderr, "Error: choose_child_counts received NULL argument.\n");
        exit(1);
    }

    if (m->num_faces == 0) {
        *num_children = 0;
        return;
    }

    total_children = (int)ceil((double)m->num_faces / TARGET_ITEMS_PER_CHILD);
    total_children = clamp(total_children, MIN_CHILDREN, MAX_CHILDREN);

    *num_children = total_children;
}

static int workers_in_range(const Model *m) {
    int suggested_children;

    if (m == NULL) {
        fprintf(stderr, "Error: workers_in_range received NULL model pointer.\n");
        return 0;
    }

    if (m->num_faces < MIN_CHILDREN) {
        fprintf(stderr,
                "Error: too few faces. The model must contain at least %d faces.\n",
                MIN_CHILDREN);
        return 0;
    }

    suggested_children = (int)ceil((double)m->num_faces / TARGET_ITEMS_PER_CHILD);

    if (suggested_children > MAX_CHILDREN) {
        fprintf(stderr,
                "Error: too many faces. This model would require more than %d children.\n",
                MAX_CHILDREN);
        return 0;
    }

    return 1;
}

/*
* Helper which clears the screen, inserts all returned ScreenVertex into the given screen buffer,
* and prints to stdout.
*/
void compose_display(char screen[HEIGHT][WIDTH], int (*pipes)[2], pid_t *pids, int num_children) {
    memset(screen, ' ', HEIGHT * WIDTH);
    
    for (int i = 0; i < num_children; i++) {
        ScreenVertex v;
        while (read(pipes[i][0], &v, sizeof(ScreenVertex)) > 0) {
            int col = (int)v.x;
            int row = (int)v.y;
            if (col >= 0 && col < WIDTH && row >= 0 && row < HEIGHT) {
                screen[row][col] = v.c;
            }
        }
    }

    /* close parent read ends */
    for (int i = 0; i < num_children; i++) {
        if (close(pipes[i][0]) == -1) {
            perror("close");
        }
    }

    /* parent waits for all children */
    for (int i = 0; i < num_children; i++) {
        if (waitpid(pids[i], NULL, 0) == -1) {
            perror("waitpid");
        }
    }

    /* print to screen */
    for (int row = 0; row < HEIGHT; row++) {
        for (int col = 0; col < WIDTH; col++) {
            putchar(screen[row][col]);
        }
        putchar('\n');
    }
}

/*
* Spawns child processes to process face vertices
*/
void spawn_workers(const Model *m, int frame) {
    int num_children;
    int pipes[MAX_CHILDREN][2];
    pid_t pids[MAX_CHILDREN];

    if (m == NULL) {
        fprintf(stderr, "Error: spawn_workers received NULL model pointer.\n");
        return;
    }

    if (!workers_in_range(m)) {
        return;
    }

    choose_child_counts(m, &num_children);
    printf("Child processes: %d\n", num_children);

    if (num_children == 0) {
        return;
    }

    /* spawn child processes */
    for (int i = 0; i < num_children; i++) {
        Range r = make_range(i, num_children, m->num_faces);

        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            close(pipes[i][0]);
            close(pipes[i][1]);
            exit(1);
        }

        if (pids[i] == 0) {
            if (close(pipes[i][0]) == -1) {
                perror("close");
                _exit(1);
            }

            write_screen_vertices((Model *)m, r.start, r.end, pipes[i][1], frame);

            if (close(pipes[i][1]) == -1) {
                perror("close");
                _exit(1);
            }
            _exit(0);
        }

        if (close(pipes[i][1]) == -1) {
            perror("close");
            exit(1);
        }
    }

    // print to screen
    char screen[HEIGHT][WIDTH];
    compose_display(screen, pipes, pids, num_children);
}

void free_model(Model *m) {
    if (m == NULL) {
        return;
    }

    free(m->vertices);
    free(m->faces);
    free(m);
}

// ==============================================================

int main(void) {
    Model *m = load_obj("../boat.obj");
    if (m == NULL) {
        return 1;
    }

    printf("Vertices: %d\n", m->num_vertices);
    printf("Faces: %d\n", m->num_faces);

    for (int i = 0; i < 5 && i < m->num_vertices; i++) {
        printf("v[%d] = (%f, %f, %f)\n",
               i,
               m->vertices[i].x,
               m->vertices[i].y,
               m->vertices[i].z);
    }

    for (int i = 0; i < 5 && i < m->num_faces; i++) {
        printf("f[%d] = (%d, %d, %d)\n",
               i,
               m->faces[i].v1,
               m->faces[i].v2,
               m->faces[i].v3);
    }

    //print each frame
    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        printf("\033[H\033[J");  // using ASCII escape character to clear screen
        spawn_workers(m, frame);
        usleep(FRAME_RATE);           
    }
    free_model(m);

    return 0;
}