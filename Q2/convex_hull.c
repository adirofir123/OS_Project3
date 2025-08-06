#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h> // For profiling

// Structure to represent a 2D point with x and y coordinates
typedef struct {
    float x;
    float y;
} Point;

// Node structure for linked list implementation
typedef struct Node {
    Point data;
    struct Node* next;
} Node;

/**
 * Creates a new node with given point data
 */
Node* create_node(Point p) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    new_node->data = p;
    new_node->next = NULL;
    return new_node;
}

/**
 * Determines the orientation of three ordered points (p, q, r)
 * Returns:
 * 0 - Colinear
 * 1 - Clockwise orientation
 * 2 - Counterclockwise orientation
 */
int orientation(Point p, Point q, Point r) {
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (fabs(val) < 1e-9) return 0;
    return (val > 0) ? 1 : 2;
}

/**
 * Comparison function for qsort() to sort points
 * First by x-coordinate, then by y-coordinate
 */
int compare_points(const void *a, const void *b) {
    Point *p1 = (Point *)a;
    Point *p2 = (Point *)b;
    if (p1->x != p2->x)
        return (p1->x > p2->x) ? 1 : -1;
    return (p1->y > p2->y) ? 1 : -1;
}

/**
 * Calculates the area of a polygon given its vertices
 * using the shoelace formula
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
 * Computes convex hull using array implementation (original)
 */
float convex_hull_array(Point points[], int n) {
    float area = 0;
    for (size_t i = 0; i < 1000; i++){
        
        if (n < 3) {
            area = calculate_polygon_area(points, n);
            return area;
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

        area = calculate_polygon_area(hull, k-1);
        free(hull);
    }
    
    return area;
}

/**
 * Computes convex hull using linked list implementation
 */
float convex_hull_linked_list(Point points[], int n) {
    float area = 0;
    for (size_t i = 0; i < 1000; i++)
        {
        
        if (n < 3) {
            area = calculate_polygon_area(points, n);
            return area;
        }

        qsort(points, n, sizeof(Point), compare_points);

        // Initialize linked list - removed unused 'head' variable
        Node* tail = NULL;
        int size = 0;

        // Build lower hull
        for (int i = 0; i < n; i++) {
            while (size >= 2) {
                Node* p2 = tail;
                Node* p1 = p2->next;
                
                Point a = p1->data;
                Point b = p2->data;
                Point c = points[i];
                
                if (orientation(a, b, c) == 2)
                    break;
                    
                // Remove last point
                tail = p1;
                free(p2);
                size--;
            }
            
            // Add new point
            Node* new_node = create_node(points[i]);
            new_node->next = tail;
            tail = new_node;
            size++;
        }

        // Build upper hull
        for (int i = n-2, t = size+1; i >= 0; i--) {
            while (size >= t) {
                Node* p2 = tail;
                Node* p1 = p2->next;
                
                Point a = p1->data;
                Point b = p2->data;
                Point c = points[i];
                
                if (orientation(a, b, c) == 2)
                    break;
                    
                // Remove last point
                tail = p1;
                free(p2);
                size--;
            }
            
            // Add new point
            Node* new_node = create_node(points[i]);
            new_node->next = tail;
            tail = new_node;
            size++;
        }

        // Convert linked list to array for area calculation
        Point* hull = (Point*)malloc(size * sizeof(Point));
        Node* current = tail;
        for (int i = 0; i < size; i++) {
            hull[i] = current->data;
            current = current->next;
        }

        area = calculate_polygon_area(hull, size);
        
        // Free linked list
        while (tail) {
            Node* temp = tail;
            tail = tail->next;
            free(temp);
        }
        free(hull);
    }
    return area;
}

int main() {
    int num_points;
    
    // Read number of points from input
    if (scanf("%d\n", &num_points) != 1 || num_points <= 0) {
        printf("Invalid number of points\n");
        return 1;
    }

    // Allocate memory for points array
    Point *points = (Point *)malloc(num_points * sizeof(Point));
    if (!points) {
        printf("Memory allocation failed\n");
        return 1;
    }

    // Read each point from input
    for (int i = 0; i < num_points; i++) {
        char line[100];
        if (!fgets(line, sizeof(line), stdin)) {
            printf("Error reading point %d\n", i);
            free(points);
            return 1;
        }

        // Clean input by removing whitespace and parentheses
        char *clean_line = line;
        while (*clean_line && (*clean_line == ' ' || *clean_line == '(' || *clean_line == ')')) {
            clean_line++;
        }

        // Split into x and y coordinates
        char *comma = strchr(clean_line, ',');
        if (!comma) {
            printf("Invalid input format for point %d\n", i);
            free(points);
            return 1;
        }

        *comma = '\0';
        points[i].x = atof(clean_line);
        points[i].y = atof(comma + 1);
    }

    printf("\nRunning convex hull algorithms...\n\n");
    
    // Run array implementation
    printf("Array implementation:\n");
    clock_t start = clock();

    float area_array = convex_hull_array(points, num_points);
    printf("Area: %.1f\n\n", area_array);

    clock_t end = clock();
    printf("Array implementation time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);
    
    // Run linked list implementation
    printf("Linked list implementation:\n");
    start = clock();
    float area_list = convex_hull_linked_list(points, num_points);
    printf("Area: %.1f\n\n", area_list);
    
    end = clock();
    printf("Linked list implementation time: %f seconds\n", ((double)(end - start)) / CLOCKS_PER_SEC);
    
    
    free(points);
    return 0;
}