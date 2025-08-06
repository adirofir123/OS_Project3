#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "proactor.h"

#define PORT "9034"
#define BACKLOG 10
#define MAX_POINTS 1000
#define AREA_THRESHOLD 100.0

typedef struct
{
    double x, y;
} Point;

Point graph_points[MAX_POINTS];
int point_count = 0;

pthread_mutex_t graph_mutex = PTHREAD_MUTEX_INITIALIZER;

double last_area = 0.0;
bool triggered_above = false;
bool triggered_below = false;

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t queue_cond = PTHREAD_COND_INITIALIZER;

void enqueue_signal()
{
    pthread_mutex_lock(&queue_mutex);
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mutex);
}

int orientation(Point p, Point q, Point r)
{
    double val = (q.y - p.y) * (r.x - q.x) -
                 (q.x - p.x) * (r.y - q.y);
    if (fabs(val) < 1e-9)
        return 0;
    return (val > 0) ? 1 : 2;
}

int compare(const void *a, const void *b)
{
    Point *p1 = (Point *)a;
    Point *p2 = (Point *)b;
    if (p1->x != p2->x)
        return (p1->x < p2->x) ? -1 : 1;
    return (p1->y < p2->y) ? -1 : 1;
}

int convex_hull(Point *points, int n, Point *hull)
{
    if (n < 3)
        return 0;
    qsort(points, n, sizeof(Point), compare);

    int m = 0;
    for (int i = 0; i < n; i++)
    {
        while (m >= 2 && orientation(hull[m - 2], hull[m - 1], points[i]) != 2)
            m--;
        hull[m++] = points[i];
    }

    for (int i = n - 2, t = m + 1; i >= 0; i--)
    {
        while (m >= t && orientation(hull[m - 2], hull[m - 1], points[i]) != 2)
            m--;
        hull[m++] = points[i];
    }

    return m - 1;
}

double calculate_polygon_area(Point *polygon, int n)
{
    double area = 0.0;
    for (int i = 0; i < n; i++)
    {
        Point p1 = polygon[i];
        Point p2 = polygon[(i + 1) % n];
        area += (p1.x * p2.y) - (p2.x * p1.y);
    }
    return fabs(area) / 2.0;
}

void *consumer_thread(void *arg)
{
    (void)arg;
    while (1)
    {
        pthread_mutex_lock(&queue_mutex);
        pthread_cond_wait(&queue_cond, &queue_mutex);
        pthread_mutex_unlock(&queue_mutex);

        pthread_mutex_lock(&graph_mutex);
        Point hull[MAX_POINTS];
        int hsize = convex_hull(graph_points, point_count, hull);
        double area = calculate_polygon_area(hull, hsize);

        if (area >= AREA_THRESHOLD && !triggered_above)
        {
            triggered_above = true;
            triggered_below = false;
            printf("At Least %.0f units belongs to CH\n", AREA_THRESHOLD);
            fflush(stdout);
        }
        else if (area < AREA_THRESHOLD && !triggered_below)
        {
            triggered_below = true;
            triggered_above = false;
            printf("At Least %.0f units no longer belongs to CH\n", AREA_THRESHOLD);
            fflush(stdout);
        }

        last_area = area;
        pthread_mutex_unlock(&graph_mutex);
    }
    return NULL;
}

void *handle_client(int client_fd)
{
    FILE *client = fdopen(client_fd, "r+");
    if (!client)
    {
        perror("fdopen");
        close(client_fd);
        return NULL;
    }
    printf("New client connected on socket %d (Thread %lu)\n", client_fd, pthread_self());
    fflush(stdout);

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), client))
    {
        if (strncmp(buffer, "Newgraph", 8) == 0)
        {
            pthread_mutex_lock(&graph_mutex);
            point_count = 0;
            pthread_mutex_unlock(&graph_mutex);
            fprintf(client, "Ready to receive points\n");
        }
        else if (strncmp(buffer, "CH", 2) == 0)
        {
            pthread_mutex_lock(&graph_mutex);
            Point hull[MAX_POINTS];
            int hsize = convex_hull(graph_points, point_count, hull);
            double area = calculate_polygon_area(hull, hsize);
            fprintf(client, "Area: %.1f\n", area);
            pthread_mutex_unlock(&graph_mutex);
            enqueue_signal();
        }
        else if (strncmp(buffer, "Newpoint", 8) == 0)
        {
            double x, y;
            sscanf(buffer + 8, "%lf,%lf", &x, &y);
            pthread_mutex_lock(&graph_mutex);
            if (point_count < MAX_POINTS)
            {
                graph_points[point_count++] = (Point){x, y};
                fprintf(client, "Point added\n");
            }
            else
            {
                fprintf(client, "Max points reached\n");
            }
            pthread_mutex_unlock(&graph_mutex);
        }
        else if (strncmp(buffer, "Removepoint", 11) == 0)
        {
            double x, y;
            sscanf(buffer + 11, "%lf,%lf", &x, &y);
            pthread_mutex_lock(&graph_mutex);
            bool found = false;
            for (int i = 0; i < point_count; i++)
            {
                if (fabs(graph_points[i].x - x) < 1e-9 && fabs(graph_points[i].y - y) < 1e-9)
                {
                    graph_points[i] = graph_points[--point_count];
                    found = true;
                    break;
                }
            }
            pthread_mutex_unlock(&graph_mutex);
            fprintf(client, found ? "Point removed\n" : "Point not found\n");
        }
        else
        {
            fprintf(client, "Unknown command\n");
        }
        fflush(client);
    }

    fclose(client);
    return NULL;
}

int main()
{
    struct addrinfo hints, *servinfo, *p;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)
    {
        perror("getaddrinfo");
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(sockfd);
    }

    if (!p)
    {
        fprintf(stderr, "Failed to bind\n");
        exit(2);
    }

    freeaddrinfo(servinfo);
    listen(sockfd, BACKLOG);

    printf("Server is up and listening on port %s (PID %d)\n", PORT, getpid());
    fflush(stdout);

    pthread_t consumer;
    pthread_create(&consumer, NULL, consumer_thread, NULL);
    startProactor(sockfd, handle_client);
    pthread_join(consumer, NULL);
    return 0;
}
