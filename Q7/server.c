#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>

#define PORT "9034"
#define BACKLOG 10

typedef struct {
    float x;
    float y;
} Point;

Point* graph_points = NULL;
int num_points = 0;
pthread_mutex_t graph_mutex = PTHREAD_MUTEX_INITIALIZER;

int orientation(Point p, Point q, Point r) {
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (fabs(val) < 1e-9) return 0;
    return (val > 0) ? 1 : 2;
}

int compare_points(const void *a, const void *b) {
    Point *p1 = (Point *)a;
    Point *p2 = (Point *)b;
    if (p1->x != p2->x)
        return (p1->x > p2->x) ? 1 : -1;
    return (p1->y > p2->y) ? 1 : -1;
}

float calculate_polygon_area(Point points[], int n) {
    float area = 0.0;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += (points[i].x * points[j].y) - (points[j].x * points[i].y);
    }
    return fabs(area) / 2.0;
}

float convex_hull_array(Point points[], int n) {
    if (n < 3) return calculate_polygon_area(points, n);
    qsort(points, n, sizeof(Point), compare_points);
    Point* hull = malloc(2 * n * sizeof(Point));
    int k = 0;
    for (int i = 0; i < n; i++) {
        while (k >= 2 && orientation(hull[k-2], hull[k-1], points[i]) != 2)
            k--;
        hull[k++] = points[i];
    }
    for (int i = n-2, t = k+1; i >= 0; i--) {
        while (k >= t && orientation(hull[k-2], hull[k-1], points[i]) != 2)
            k--;
        hull[k++] = points[i];
    }
    float area = calculate_polygon_area(hull, k-1);
    free(hull);
    return area;
}

void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);
    char buf[256];

    while (1) {
        int bytes = recv(client_fd, buf, sizeof(buf)-1, 0);
        if (bytes <= 0) break;
        buf[bytes] = '\0';

        if (strncmp(buf, "Newgraph", 8) == 0) {
            int n;
            if (sscanf(buf + 8, "%d", &n) == 1 && n > 0) {
                pthread_mutex_lock(&graph_mutex);
                if (graph_points) free(graph_points);
                graph_points = malloc(n * sizeof(Point));
                num_points = n;
                pthread_mutex_unlock(&graph_mutex);
                send(client_fd, "Ready to receive points\n", 25, 0);

                for (int i = 0; i < n; i++) {
                    int r = recv(client_fd, buf, sizeof(buf)-1, 0);
                    if (r <= 0) break;
                    buf[r] = '\0';
                    float x, y;
                    if (sscanf(buf, "%f,%f", &x, &y) == 2) {
                        pthread_mutex_lock(&graph_mutex);
                        graph_points[i].x = x;
                        graph_points[i].y = y;
                        pthread_mutex_unlock(&graph_mutex);
                    }
                }
                send(client_fd, "Graph created successfully\n", 28, 0);
            } else {
                send(client_fd, "Invalid Newgraph command\n", 26, 0);
            }
        } else if (strncmp(buf, "CH", 2) == 0) {
            pthread_mutex_lock(&graph_mutex);
            if (num_points == 0) {
                pthread_mutex_unlock(&graph_mutex);
                send(client_fd, "No points in graph\n", 20, 0);
            } else {
                float area = convex_hull_array(graph_points, num_points);
                pthread_mutex_unlock(&graph_mutex);
                char res[50];
                snprintf(res, sizeof(res), "Area: %.1f\n", area);
                send(client_fd, res, strlen(res), 0);
            }
        } else if (strncmp(buf, "Newpoint", 8) == 0) {
            float x, y;
            if (sscanf(buf + 8, "%f,%f", &x, &y) == 2) {
                pthread_mutex_lock(&graph_mutex);
                graph_points = realloc(graph_points, (num_points + 1) * sizeof(Point));
                graph_points[num_points].x = x;
                graph_points[num_points].y = y;
                num_points++;
                pthread_mutex_unlock(&graph_mutex);
                send(client_fd, "Point added\n", 13, 0);
            } else {
                send(client_fd, "Invalid Newpoint command\n", 27, 0);
            }
        } else if (strncmp(buf, "Removepoint", 11) == 0) {
            float x, y;
            if (sscanf(buf + 11, "%f,%f", &x, &y) == 2) {
                bool found = false;
                pthread_mutex_lock(&graph_mutex);
                for (int i = 0; i < num_points; i++) {
                    if (fabs(graph_points[i].x - x) < 1e-9 && fabs(graph_points[i].y - y) < 1e-9) {
                        for (int j = i; j < num_points - 1; j++)
                            graph_points[j] = graph_points[j + 1];
                        num_points--;
                        graph_points = realloc(graph_points, num_points * sizeof(Point));
                        found = true;
                        break;
                    }
                }
                pthread_mutex_unlock(&graph_mutex);
                send(client_fd, found ? "Point removed\n" : "Point not found\n", found ? 15 : 18, 0);
            } else {
                send(client_fd, "Invalid Removepoint command\n", 30, 0);
            }
        } else {
            send(client_fd, "Unknown command\n", 17, 0);
        }
    }

    close(client_fd);
    return NULL;
}

int main() {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    int yes = 1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &servinfo) != 0) {
        perror("getaddrinfo");
        exit(1);
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) exit(1);
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) { close(sockfd); continue; }
        break;
    }

    freeaddrinfo(servinfo);
    if (!p) exit(1);
    if (listen(sockfd, BACKLOG) == -1) exit(1);

    printf("server: waiting for connections...\n");

    while (1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        printf("New connection accepted: socket %d\n", new_fd);
        if (new_fd == -1) continue;
        pthread_t tid;
        int* pclient = malloc(sizeof(int));
        *pclient = new_fd;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    return 0;
}
