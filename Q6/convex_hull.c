#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

// Structure to represent a 2D point with x and y coordinates
typedef struct {
    float x;
    float y;
} Point;

// Node structure for linked list implementation
typedef struct Node {
    Point data;          // Point data stored in the node
    struct Node* next;   // Pointer to the next node
} Node;

/**
 * Creates a new node with given point data
 * @param p The point to store in the node
 * @return Pointer to the newly created node
 */
Node* create_node(Point p) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->data = p;
    new_node->next = NULL;
    return new_node;
}

/**
 * Determines the orientation of three ordered points (p, q, r)
 * @param p First point
 * @param q Second point
 * @param r Third point
 * @return 0 if colinear, 1 if clockwise, 2 if counterclockwise
 */
int orientation(Point p, Point q, Point r) {
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (fabs(val) < 1e-9) return 0;   // Colinear
    return (val > 0) ? 1 : 2;          // Clockwise or counterclockwise
}

/**
 * Comparison function for qsort() to sort points
 * First by x-coordinate, then by y-coordinate
 * @param a First point to compare
 * @param b Second point to compare
 * @return Comparison result for sorting
 */
int compare_points(const void *a, const void *b) {
    Point *p1 = (Point *)a;
    Point *p2 = (Point *)b;
    if (p1->x != p2->x)
        return (p1->x > p2->x) ? 1 : -1;
    return (p1->y > p2->y) ? 1 : -1;
}

/**
 * Calculates the area of a polygon using the shoelace formula
 * @param points Array of points representing the polygon
 * @param n Number of points
 * @return Calculated area of the polygon
 */
float calculate_polygon_area(Point points[], int n) {
    float area = 0.0;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += (points[i].x * points[j].y) - (points[j].x * points[i].y);
    }
    return fabs(area) / 2.0;
}

/**
 * Computes convex hull using array implementation
 * @param points Array of input points
 * @param n Number of points
 * @return Area of the convex hull
 */
float convex_hull_array(Point points[], int n) {
    if (n < 3) {
        return calculate_polygon_area(points, n);
    }

    // Sort points by x-coordinate (then y-coordinate)
    qsort(points, n, sizeof(Point), compare_points);

    Point* hull = (Point*)malloc(n * 2 * sizeof(Point));
    int k = 0;

    // Build lower hull
    for (int i = 0; i < n; i++) {
        // Remove points that would create clockwise turn
        while (k >= 2 && orientation(hull[k-2], hull[k-1], points[i]) != 2)
            k--;
        hull[k++] = points[i];
    }

    // Build upper hull
    for (int i = n-2, t = k+1; i >= 0; i--) {
        // Remove points that would create clockwise turn
        while (k >= t && orientation(hull[k-2], hull[k-1], points[i]) != 2)
            k--;
        hull[k++] = points[i];
    }

    float area = calculate_polygon_area(hull, k-1);
    free(hull);
    return area;
}

// Global variables to store the current graph state
Point* points = NULL;    // Dynamic array of points
int num_points = 0;      // Current number of points

/**
 * Handles the Newgraph command - initializes a new graph with n points
 * @param n Number of points to read
 */
void handle_newgraph(int n) {
    // Free previous points if any
    if (points != NULL) {
        free(points);
        points = NULL;
    }

    num_points = n;
    points = (Point *)malloc(n * sizeof(Point));
    if (!points) {
        printf("Memory allocation failed\n");
        exit(1);
    }

    // Read n points from input
    for (int i = 0; i < n; i++) {
        char line[100];
        if (!fgets(line, sizeof(line), stdin)) {
            printf("Error reading point %d\n", i);
            free(points);
            exit(1);
        }

        // Parse x and y coordinates
        float x, y;
        if (sscanf(line, "%f,%f", &x, &y) != 2) {
            printf("Invalid point format: %s\n", line);
            free(points);
            exit(1);
        }
        points[i].x = x;
        points[i].y = y;
    }
}

/**
 * Handles the Newpoint command - adds a new point to the graph
 * @param x X-coordinate of new point
 * @param y Y-coordinate of new point
 */
void handle_newpoint(float x, float y) {
    num_points++;
    points = (Point *)realloc(points, num_points * sizeof(Point));
    points[num_points - 1].x = x;
    points[num_points - 1].y = y;
}

/**
 * Handles the Removepoint command - removes a point from the graph
 * @param x X-coordinate of point to remove
 * @param y Y-coordinate of point to remove
 * @return true if point was found and removed, false otherwise
 */
bool handle_removepoint(float x, float y) {
    for (int i = 0; i < num_points; i++) {
        // Compare with tolerance for floating point numbers
        if (fabs(points[i].x - x) < 1e-9 && fabs(points[i].y - y) < 1e-9) {
            // Shift all points after the removed one
            for (int j = i; j < num_points - 1; j++) {
                points[j] = points[j + 1];
            }
            num_points--;
            points = (Point *)realloc(points, num_points * sizeof(Point));
            return true;
        }
    }
    return false; // Point not found
}

/**
 * Handles the CH command - computes and prints convex hull area
 */
void handle_ch() {
    if (num_points == 0) {
        printf("No points in graph\n");
        return;
    }

    float area = convex_hull_array(points, num_points);
    printf("Area: %.1f\n", area);
}

int main() {
    char command[50];

    // Main command loop
    while (fgets(command, sizeof(command), stdin)) {
        // Process Newgraph command
        if (strncmp(command, "Newgraph", 8) == 0) {
            int n;
            if (sscanf(command + 8, "%d", &n) != 1 || n <= 0) {
                printf("Invalid Newgraph command\n");
                continue;
            }
            handle_newgraph(n);
        }
        // Process CH command
        else if (strncmp(command, "CH", 2) == 0) {
            handle_ch();
        }
        // Process Newpoint command
        else if (strncmp(command, "Newpoint", 8) == 0) {
            float x, y;
            if (sscanf(command + 8, "%f,%f", &x, &y) != 2) {
                printf("Invalid Newpoint command\n");
                continue;
            }
            handle_newpoint(x, y);
        }
        // Process Removepoint command
        else if (strncmp(command, "Removepoint", 11) == 0) {
            float x, y;
            if (sscanf(command + 11, "%f,%f", &x, &y) != 2) {
                printf("Invalid Removepoint command\n");
                continue;
            }
            if (!handle_removepoint(x, y)) {
                printf("Point not found\n");
            }
        }
        else {
            printf("Unknown command\n");
        }
    }

    // Clean up memory before exiting
    if (points != NULL) {
        free(points);
    }

    return 0;
}