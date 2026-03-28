#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>

#define MIN_CHILDREN 3
#define MAX_CHILDREN 10
#define TARGET_ITEMS_PER_CHILD 3000

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

typedef struct {
    int start;
    int end;
} Range;

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

/* parse_face_line  only accepts 4-edge faces*/
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

static void choose_child_counts(const Model *m, int *vertex_children, int *face_children) {
    int total_work;
    int total_children;
    int vc = 0;
    int fc = 0;

    if (m == NULL || vertex_children == NULL || face_children == NULL) {
        fprintf(stderr, "Error: choose_child_counts received NULL argument.\n");
        exit(1);
    }

    total_work = m->num_vertices + m->num_faces;

    if (total_work == 0) {
        *vertex_children = 0;
        *face_children = 0;
        return;
    }

    total_children = (int)ceil((double)total_work / TARGET_ITEMS_PER_CHILD);
    total_children = clamp(total_children, MIN_CHILDREN, MAX_CHILDREN);

    if (m->num_vertices > 0 && m->num_faces > 0) {
        vc = (int)round((double)total_children * m->num_vertices / total_work);
        fc = total_children - vc;

        if (vc == 0) {
            vc = 1;
            fc--;
        }
        if (fc == 0) {
            fc = 1;
            vc--;
        }
    } else if (m->num_vertices > 0) {
        vc = total_children;
        fc = 0;
    } else {
        vc = 0;
        fc = total_children;
    }

    *vertex_children = vc;
    *face_children = fc;
}

static int workers_in_range(const Model *m) {
    int total_work;
    int suggested_children;

    if (m == NULL) {
        fprintf(stderr, "Error: workers_in_range received NULL model pointer.\n");
        return 0;
    }

    total_work = m->num_vertices + m->num_faces;

    if (total_work < 3) {
        fprintf(stderr,
                "Error: too little work. The model must contain at least 3 combined vertices/faces.\n");
        return 0;
    }

    suggested_children = (int)ceil((double)total_work / TARGET_ITEMS_PER_CHILD);

    if (suggested_children > MAX_CHILDREN) {
        fprintf(stderr,
                "Error: too much work. This model would require more than %d children.\n",
                MAX_CHILDREN);
        return 0;
    }

    return 1;
}


void spawn_workers(const Model *m) {
    int vertex_children, face_children;
    int total_children;
    int pipes[MAX_CHILDREN][2];
    pid_t pids[MAX_CHILDREN];
    int child_index = 0;

    if (m == NULL) {
        fprintf(stderr, "Error: spawn_workers received NULL model pointer.\n");
        return;
    }

    if (!workers_in_range(m)) {
        return;
    }

    choose_child_counts(m, &vertex_children, &face_children);
    total_children = vertex_children + face_children;

    /* mainly for debugging*/
    printf("Vertex children: %d\n", vertex_children);
    printf("Face children: %d\n", face_children);
    printf("Total children: %d\n", total_children);

    if (total_children == 0) {
        return;
    }

    /* spawn vertex children */
    for (int i = 0; i < vertex_children; i++) {
        Range r = make_range(i, vertex_children, m->num_vertices);

        if (pipe(pipes[child_index]) == -1) {
            perror("pipe");
            exit(1);
        }

        pids[child_index] = fork();
        if (pids[child_index] < 0) {
            perror("fork");
            close(pipes[child_index][0]);
            close(pipes[child_index][1]);
            exit(1);
        }

        if (pids[child_index] == 0) {
            if (close(pipes[child_index][0]) == -1) {
                perror("close");
                _exit(1);
            }

            /* TODO: call vertex function here */


            if (close(pipes[child_index][1]) == -1) {
                perror("close");
                _exit(1);
            }
            _exit(0);
        }

        if (close(pipes[child_index][1]) == -1) {
            perror("close");
            exit(1);
        }

        child_index++;
    }

    /* spawn face children */
    for (int i = 0; i < face_children; i++) {
        Range r = make_range(i, face_children, m->num_faces);

        if (pipe(pipes[child_index]) == -1) {
            perror("pipe");
            exit(1);
        }

        pids[child_index] = fork();
        if (pids[child_index] < 0) {
            perror("fork");
            close(pipes[child_index][0]);
            close(pipes[child_index][1]);
            exit(1);
        }

        if (pids[child_index] == 0) {
            if (close(pipes[child_index][0]) == -1) {
                perror("close");
                _exit(1);
            }

            /* TODO: call face function here */


            if (close(pipes[child_index][1]) == -1) {
                perror("close");
                _exit(1);
            }
            _exit(0);
        }

        if (close(pipes[child_index][1]) == -1) {
            perror("close");
            exit(1);
        }

        child_index++;
    }

    /* TODO: parent reads child results here */


    /* close parent read ends after result handling is implemented */
    for (int i = 0; i < total_children; i++) {
        if (close(pipes[i][0]) == -1) {
            perror("close");
        }
    }

    /* parent waits for all children */
    for (int i = 0; i < total_children; i++) {
        if (waitpid(pids[i], NULL, 0) == -1) {
            perror("waitpid");
        }
    }
}

void free_model(Model *m) {
    if (m == NULL) {
        return;
    }

    free(m->vertices);
    free(m->faces);
    free(m);
}


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

    spawn_workers(m);
    free_model(m);

    return 0;
}