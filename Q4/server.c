#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

#define PORT "9034"   // Port we're listening on
#define BACKLOG 10    // Max connections waiting in queue

// Structure for 2D point
typedef struct {
    float x;
    float y;
} Point;

// Global graph state
Point* graph_points = NULL;
int num_points = 0;

// Function declarations
int orientation(Point p, Point q, Point r);
int compare_points(const void *a, const void *b);
float calculate_polygon_area(Point points[], int n);
float convex_hull_array(Point points[], int n);
void handle_newgraph(int n, int fd);
void handle_newpoint(float x, float y);
bool handle_removepoint(float x, float y);
void handle_ch(int fd);
void *get_in_addr(struct sockaddr *sa);
int send_all(int fd, char *buf, int len);

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Send all data to client
int send_all(int fd, char *buf, int len) {
    int total = 0;
    int bytesleft = len;
    int n = 0;

    while(total < len) {
        n = send(fd, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    return n==-1 ? -1 : 0;
}

// Orientation function (same as before)
int orientation(Point p, Point q, Point r) {
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (fabs(val) < 1e-9) return 0;
    return (val > 0) ? 1 : 2;
}

// Comparison function for sorting points
int compare_points(const void *a, const void *b) {
    Point *p1 = (Point *)a;
    Point *p2 = (Point *)b;
    if (p1->x != p2->x)
        return (p1->x > p2->x) ? 1 : -1;
    return (p1->y > p2->y) ? 1 : -1;
}

// Calculate polygon area
float calculate_polygon_area(Point points[], int n) {
    float area = 0.0;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += (points[i].x * points[j].y) - (points[j].x * points[i].y);
    }
    return fabs(area) / 2.0;
}

// Convex hull calculation
float convex_hull_array(Point points[], int n) {
    if (n < 3) {
        return calculate_polygon_area(points, n);
    }

    qsort(points, n, sizeof(Point), compare_points);

    Point* hull = (Point*)malloc(n * 2 * sizeof(Point));
    int k = 0;

    // Build lower hull
    for (int i = 0; i < n; i++) {
        while (k >= 2 && orientation(hull[k-2], hull[k-1], points[i]) != 2)
            k--;
        hull[k++] = points[i];
    }

    // Build upper hull
    for (int i = n-2, t = k+1; i >= 0; i--) {
        while (k >= t && orientation(hull[k-2], hull[k-1], points[i]) != 2)
            k--;
        hull[k++] = points[i];
    }

    float area = calculate_polygon_area(hull, k-1);
    free(hull);
    return area;
}

// Handle Newgraph command
void handle_newgraph(int n, int fd) {
    // Free previous points if any
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

    // Send acknowledgment
    send(fd, "Ready to receive points\n", 24, 0);

    // Read n points from client
    for (int i = 0; i < n; i++) {
        char buf[256];
        int bytes_received = recv(fd, buf, sizeof(buf)-1, 0);
        if (bytes_received <= 0) {
            // Connection closed or error
            free(graph_points);
            graph_points = NULL;
            num_points = 0;
            return;
        }
        buf[bytes_received] = '\0';

        // Parse point
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

// Handle Newpoint command
void handle_newpoint(float x, float y) {
    num_points++;
    graph_points = (Point *)realloc(graph_points, num_points * sizeof(Point));
    graph_points[num_points - 1].x = x;
    graph_points[num_points - 1].y = y;
}

// Handle Removepoint command
bool handle_removepoint(float x, float y) {
    for (int i = 0; i < num_points; i++) {
        if (fabs(graph_points[i].x - x) < 1e-9 && fabs(graph_points[i].y - y) < 1e-9) {
            // Shift points
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

// Handle CH command
void handle_ch(int fd) {
    if (num_points == 0) {
        send(fd, "No points in graph\n", 19, 0);
        return;
    }

    float area = convex_hull_array(graph_points, num_points);
    char response[50];
    snprintf(response, sizeof(response), "Area: %.1f\n", area);
    send(fd, response, strlen(response), 0);
}

// Main server function
int main(void) {
    int sockfd, new_fd;  // Listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // Connector's address information
    socklen_t sin_size;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // Use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Loop through all results and bind to first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
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

    freeaddrinfo(servinfo); // All done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    // Main accept() loop
    while(1) {
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        // Handle new connection in a loop
        while(1) {
            char buf[256];
            int bytes_received = recv(new_fd, buf, sizeof(buf)-1, 0);
            if (bytes_received <= 0) {
                // Connection closed or error
                break;
            }
            buf[bytes_received] = '\0';

            // Process command
            if (strncmp(buf, "Newgraph", 8) == 0) {
                int n;
                if (sscanf(buf + 8, "%d", &n) == 1 && n > 0) {
                    handle_newgraph(n, new_fd);
                } else {
                    send(new_fd, "Invalid Newgraph command\n", 25, 0);
                }
            }
            else if (strncmp(buf, "CH", 2) == 0) {
                handle_ch(new_fd);
            }
            else if (strncmp(buf, "Newpoint", 8) == 0) {
                float x, y;
                if (sscanf(buf + 8, "%f,%f", &x, &y) == 2) {
                    handle_newpoint(x, y);
                    send(new_fd, "Point added\n", 12, 0);
                } else {
                    send(new_fd, "Invalid Newpoint command\n", 25, 0);
                }
            }
            else if (strncmp(buf, "Removepoint", 11) == 0) {
                float x, y;
                if (sscanf(buf + 11, "%f,%f", &x, &y) == 2) {
                    if (handle_removepoint(x, y)) {
                        send(new_fd, "Point removed\n", 14, 0);
                    } else {
                        send(new_fd, "Point not found\n", 16, 0);
                    }
                } else {
                    send(new_fd, "Invalid Removepoint command\n", 28, 0);
                }
            }
            else {
                send(new_fd, "Unknown command\n", 16, 0);
            }
        }

        close(new_fd);  // Close connection
    }

    // Cleanup (though we'll likely never get here)
    if (graph_points) free(graph_points);
    return 0;
}