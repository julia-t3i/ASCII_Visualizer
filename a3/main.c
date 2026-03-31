#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <limits.h>

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

    // try triangles first
    if (sscanf(line, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &a, &b, &c) == 3) {
        add_face(m, a - 1, b - 1, c - 1);
        return;
    }
    if (sscanf(line, "f %d//%*d %d//%*d %d//%*d", &a, &b, &c) == 3) {
        add_face(m, a - 1, b - 1, c - 1);
        return;
    }
    if (sscanf(line, "f %d/%*d %d/%*d %d/%*d", &a, &b, &c) == 3) {
        add_face(m, a - 1, b - 1, c - 1);
        return;
    }
    if (sscanf(line, "f %d %d %d", &a, &b, &c) == 3) {
        add_face(m, a - 1, b - 1, c - 1);
        return;
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

static void init_workers(const Model *m, Worker *workers, int *num_workers) {
    if (m == NULL || workers == NULL || num_workers == NULL) {
        fprintf(stderr, "Error: init_workers received NULL argument.\n");
        exit(1);
    }

    choose_child_counts(m, num_workers);
    if (*num_workers == 0) {
        fprintf(stderr, "Error: no workers configured.\n");
        exit(1);
    }

    printf("Worker processes: %d\n", *num_workers);

    for (int i = 0; i < *num_workers; i++) {
        int control_pipe[2];
        int result_pipe[2];

        if (pipe(control_pipe) == -1) {
            perror("pipe control");
            exit(1);
        }
        if (pipe(result_pipe) == -1) {
            perror("pipe result");
            exit(1);
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // child
            close(control_pipe[1]);
            close(result_pipe[0]);

            while (1) {
                TaskMsg task;
                ssize_t got = read(control_pipe[0], &task, sizeof(TaskMsg));
                if (got <= 0) {
                    break;
                }
                if (got != sizeof(TaskMsg)) {
                    continue;
                }

                write_screen_vertices(m, task.start, task.end, result_pipe[1], task.frame);

                EndMsg end_msg = { .done = 1 };
                if (write(result_pipe[1], &end_msg, sizeof(EndMsg)) != sizeof(EndMsg)) {
                    perror("write end");
                    break;
                }
            }

            close(control_pipe[0]);
            close(result_pipe[1]);
            free_model((Model *)m);
            _exit(0);
        }

        // parent
        close(control_pipe[0]);
        close(result_pipe[1]);

        workers[i].pid = pid;
        workers[i].to_fd = control_pipe[1];
        workers[i].from_fd = result_pipe[0];
        workers[i].busy = 0;
    }
}

static void cleanup_workers(Worker *workers, int num_workers) {
    for (int i = 0; i < num_workers; i++) {
        if (workers[i].to_fd >= 0) close(workers[i].to_fd);
        if (workers[i].from_fd >= 0) close(workers[i].from_fd);
    }

    for (int i = 0; i < num_workers; i++) {
        if (waitpid(workers[i].pid, NULL, 0) == -1) {
            perror("waitpid");
        }
    }
}

static void send_task_to_worker(Worker *worker, TaskMsg *task) {
    if (write(worker->to_fd, task, sizeof(TaskMsg)) != sizeof(TaskMsg)) {
        perror("write task");
        exit(1);
    }
    worker->busy = 1;
}

static void fill_screen(Worker *worker, char screen[HEIGHT][WIDTH], int *tasks_done, int *next_task, int total_tasks, TaskMsg *tasks) {
    while (1) {
        ScreenVertex vertex;
        ssize_t got = read(worker->from_fd, &vertex, sizeof(ScreenVertex));

        if (got == 0) {
            worker->busy = 0;
            return;
        }

        if (got == sizeof(ScreenVertex)) {
            int col = (int)vertex.x;
            int row = (int)vertex.y;
            if (col >= 0 && col < WIDTH && row >= 0 && row < HEIGHT) {
                screen[row][col] = vertex.c;
            }
            continue;
        }

        if (got == sizeof(EndMsg)) {
            EndMsg end;
            memcpy(&end, &vertex, sizeof(EndMsg));
            if (end.done == 1) {
                worker->busy = 0;
                (*tasks_done)++;
                if (*next_task < total_tasks) {
                    send_task_to_worker(worker, &tasks[*next_task]);
                    (*next_task)++;
                }
            }
            return;
        }

        if (got > 0 && got < (ssize_t)sizeof(ScreenVertex)) {
            if (got == sizeof(EndMsg)) {
                EndMsg end;
                memcpy(&end, &vertex, sizeof(EndMsg));
                if (end.done == 1) {
                    worker->busy = 0;
                    (*tasks_done)++;
                    if (*next_task < total_tasks) {
                        send_task_to_worker(worker, &tasks[*next_task]);
                        (*next_task)++;
                    }
                }
                return;
            }
            continue;
        }

        if (got < 0) {
            perror("read from worker");
            worker->busy = 0;
            return;
        }

        return;
    }
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
void spawn_workers(const Model *m) {
    if (m == NULL) {
        fprintf(stderr, "Error: spawn_workers received NULL model pointer.\n");
        return;
    }

    if (!workers_in_range(m)) {
        return;
    }

    Worker workers[MAX_CHILDREN];
    int num_workers = 0;
    init_workers(m, workers, &num_workers);

    int total_tasks = (m->num_faces + CHUNK_SIZE - 1) / CHUNK_SIZE;
    TaskMsg *task_list = malloc(sizeof(TaskMsg) * total_tasks);
    if (task_list == NULL) {
        perror("malloc task_list");
        cleanup_workers(workers, num_workers);
        return;
    }

    for (int i = 0; i < total_tasks; i++) {
        task_list[i].frame = 0;
        task_list[i].start = i * CHUNK_SIZE;
        task_list[i].end = (i + 1) * CHUNK_SIZE;
        if (task_list[i].end > m->num_faces) {
            task_list[i].end = m->num_faces;
        }
    }

    for (int frame = 0; frame < FRAME_COUNT; frame++) {
        char screen[HEIGHT][WIDTH];
        memset(screen, ' ', sizeof(screen));

        int next_task = 0;
        int tasks_done = 0;

        for (int i = 0; i < num_workers && next_task < total_tasks; i++) {
            task_list[next_task].frame = frame;
            send_task_to_worker(&workers[i], &task_list[next_task]);
            next_task++;
        }

        while (tasks_done < total_tasks) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            int max_fd = -1;

            for (int i = 0; i < num_workers; i++) {
                if (workers[i].from_fd >= 0) {
                    FD_SET(workers[i].from_fd, &read_fds);
                    if (workers[i].from_fd > max_fd) {
                        max_fd = workers[i].from_fd;
                    }
                }
            }

            if (max_fd < 0) {
                break;
            }

            int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
            if (ready == -1) {
                if (errno == EINTR) {
                    continue;
                }
                perror("select");
                break;
            }

            for (int i = 0; i < num_workers; i++) {
                if (workers[i].from_fd >= 0 && FD_ISSET(workers[i].from_fd, &read_fds)) {
                    fill_screen(&workers[i], screen, &tasks_done, &next_task, total_tasks, task_list);
                }
            }
        }

        for (int row = 0; row < HEIGHT; row++) {
            for (int col = 0; col < WIDTH; col++) {
                putchar(screen[row][col]);
            }
            putchar('\n');
        }

        usleep(FRAME_RATE);
    }

    free(task_list);
    cleanup_workers(workers, num_workers);
}

void free_model(Model *m) {
    if (m == NULL) {
        return;
    }

    free(m->vertices);
    free(m->faces);
    free(m);
}

void normalize_model(Model *m) {
    if (m == NULL || m->num_vertices == 0) {
        return;
    }


    float min_x = m->vertices[0].x, max_x = m->vertices[0].x;
    float min_y = m->vertices[0].y, max_y = m->vertices[0].y;
    float min_z = m->vertices[0].z, max_z = m->vertices[0].z;


    for (int i = 1; i < m->num_vertices; i++) {
        if (m->vertices[i].x < min_x) min_x = m->vertices[i].x;
        if (m->vertices[i].x > max_x) max_x = m->vertices[i].x;
        if (m->vertices[i].y < min_y) min_y = m->vertices[i].y;
        if (m->vertices[i].y > max_y) max_y = m->vertices[i].y;
        if (m->vertices[i].z < min_z) min_z = m->vertices[i].z;
        if (m->vertices[i].z > max_z) max_z = m->vertices[i].z;
    }


    float center_x = (min_x + max_x) / 2.0f;
    float center_y = (min_y + max_y) / 2.0f;
    float center_z = (min_z + max_z) / 2.0f;


    float size_x = max_x - min_x;
    float size_y = max_y - min_y;
    float size_z = max_z - min_z;


    float max_dim = size_x;
    if (size_y > max_dim) max_dim = size_y;
    if (size_z > max_dim) max_dim = size_z;


    if (max_dim == 0.0f) {
        return;
    }


    float target_size = 4.0f;


    for (int i = 0; i < m->num_vertices; i++) {
        m->vertices[i].x = ((m->vertices[i].x - center_x) / max_dim) * target_size;
        m->vertices[i].y = ((m->vertices[i].y - center_y) / max_dim) * target_size;
        m->vertices[i].z = ((m->vertices[i].z - center_z) / max_dim) * target_size;
    }
}

// ==============================================================

int main(int argc, char *argv[]) {
    const char *input = "fish";  // default 
    if (argc >= 2 && argv[1] != NULL && argv[1][0] != '\0') {
        input = argv[1];
    }

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "../%s.obj", input);

    Model *m = load_obj(path);
    if (m == NULL) {
        return 1;
    }

    normalize_model(m);

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

    // center the model
    float minx = INFINITY, maxx = -INFINITY;
    float miny = INFINITY, maxy = -INFINITY;
    float minz = INFINITY, maxz = -INFINITY;
    for (int i = 0; i < m->num_vertices; i++) {
        Vertex v = m->vertices[i];
        minx = fmin(minx, v.x);
        maxx = fmax(maxx, v.x);
        miny = fmin(miny, v.y);
        maxy = fmax(maxy, v.y);
        minz = fmin(minz, v.z);
        maxz = fmax(maxz, v.z);
    }
    float cx = (minx + maxx) / 2.0f;
    float cy = (miny + maxy) / 2.0f;
    float cz = (minz + maxz) / 2.0f;
    for (int i = 0; i < m->num_vertices; i++) {
        m->vertices[i].x -= cx;
        m->vertices[i].y -= cy;
        m->vertices[i].z -= cz;
    }

    spawn_workers(m);
    free_model(m);
    return 0;
}
