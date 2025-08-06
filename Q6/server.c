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
#include "reactor.h"

#define PORT "9034"
#define BACKLOG 10

// Structure for 2D point
typedef struct {
    float x;
    float y;
} Point;

// Global graph state
Point* graph_points = NULL;
int num_points = 0;

// Function declarations
void accept_handler(int listen_fd);
void client_handler(int client_fd);
void handle_newgraph(int n, int fd);
void handle_newpoint(float x, float y);
bool handle_removepoint(float x, float y);
void handle_ch(int fd);
void *get_in_addr(struct sockaddr *sa);

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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
    if (n < 3) {
        return calculate_polygon_area(points, n);
    }

    qsort(points, n, sizeof(Point), compare_points);

    Point* hull = (Point*)malloc(n * 2 * sizeof(Point));
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

void handle_newgraph(int n, int fd) {
    if (graph_points != NULL) {
        free(graph_points);
        graph_points = NULL;
    }

    num_points = n;
    graph_points = (Point *)malloc(n * sizeof(Point));
    if (!graph_points) {
        send(fd, "Memory allocation failed\n", 25, 0);
        return;
    }

    send(fd, "Ready to receive points\n", 24, 0);

    for (int i = 0; i < n; i++) {
        char buf[256];
        int bytes_received = recv(fd, buf, sizeof(buf)-1, 0);
        if (bytes_received <= 0) {
            free(graph_points);
            graph_points = NULL;
            num_points = 0;
            return;
        }
        buf[bytes_received] = '\0';

        float x, y;
        if (sscanf(buf, "%f,%f", &x, &y) != 2) {
            send(fd, "Invalid point format\n", 21, 0);
            free(graph_points);
            graph_points = NULL;
            num_points = 0;
            return;
        }
        graph_points[i].x = x;
        graph_points[i].y = y;
    }

    send(fd, "Graph created successfully\n", 27, 0);
}

void handle_newpoint(float x, float y) {
    num_points++;
    graph_points = (Point *)realloc(graph_points, num_points * sizeof(Point));
    graph_points[num_points - 1].x = x;
    graph_points[num_points - 1].y = y;
}

bool handle_removepoint(float x, float y) {
    for (int i = 0; i < num_points; i++) {
        if (fabs(graph_points[i].x - x) < 1e-9 && fabs(graph_points[i].y - y) < 1e-9) {
            for (int j = i; j < num_points - 1; j++) {
                graph_points[j] = graph_points[j + 1];
            }
            num_points--;
            graph_points = (Point *)realloc(graph_points, num_points * sizeof(Point));
            return true;
        }
    }
    return false;
}

void handle_ch(int fd) {
    if (num_points == 0) {
        send(fd, "No points in graph\n", 20, 0);
        return;
    }
    float area = convex_hull_array(graph_points, num_points);
    char response[50];
    snprintf(response, sizeof(response), "Area: %.1f\n", area);
    send(fd, response, strlen(response), 0);
}

void client_handler(int client_fd) {
    char buf[256];
    int bytes = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (bytes <= 0) {
        printf("Client %d disconnected\n", client_fd);
        removeFd(getCurrentReactor(), client_fd);
        close(client_fd);
        return;
    }

    buf[bytes] = '\0';

    if (strncmp(buf, "Newgraph", 8) == 0) {
        int n;
        if (sscanf(buf + 8, "%d", &n) == 1 && n > 0) {
            handle_newgraph(n, client_fd);
        } else {
            send(client_fd, "Invalid Newgraph command\n", 26, 0);
        }
    }
    else if (strncmp(buf, "CH", 2) == 0) {
        handle_ch(client_fd);
    }
    else if (strncmp(buf, "Newpoint", 8) == 0) {
        float x, y;
        if (sscanf(buf + 8, "%f,%f", &x, &y) == 2) {
            handle_newpoint(x, y);
            send(client_fd, "Point added\n", 13, 0);
        } else {
            send(client_fd, "Invalid Newpoint command\n", 27, 0);
        }
    }
    else if (strncmp(buf, "Removepoint", 11) == 0) {
        float x, y;
        if (sscanf(buf + 11, "%f,%f", &x, &y) == 2) {
            if (handle_removepoint(x, y)) {
                send(client_fd, "Point removed\n", 15, 0);
            } else {
                send(client_fd, "Point not found\n", 18, 0);
            }
        } else {
            send(client_fd, "Invalid Removepoint command\n", 30, 0);
        }
    }
    else {
        send(client_fd, "Unknown command\n", 17, 0);
    }
}

void accept_handler(int listen_fd) {
    struct sockaddr_storage their_addr;
    socklen_t sin_size = sizeof(their_addr);
    int client_fd = accept(listen_fd, (struct sockaddr*)&their_addr, &sin_size);
    if (client_fd == -1) {
        perror("accept");
        return;
    }

    printf("New connection: %d\n", client_fd);
    addFd(getCurrentReactor(), client_fd, client_handler);
}

int main(void) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int yes=1;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    Reactor* reactor = createReactor();
    if (!reactor) {
        fprintf(stderr, "Failed to create reactor\n");
        exit(1);
    }

    addFd(reactor, sockfd, accept_handler);

    while (1) {
        pause(); // keep main alive
    }

    stopReactor(reactor);
    if (graph_points) free(graph_points);
    return 0;
}