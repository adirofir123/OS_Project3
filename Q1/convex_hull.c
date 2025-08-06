#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Structure to represent a 2D point with x and y coordinates
typedef struct {
    float x;
    float y;
} Point;

/**
 * Determines the orientation of three ordered points (p, q, r)
 * Returns:
 * 0 - Colinear
 * 1 - Clockwise orientation
 * 2 - Counterclockwise orientation
 */
int orientation(Point p, Point q, Point r) {
    float val = (q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y);
    if (fabs(val) < 1e-9) return 0;  // Colinear
    return (val > 0) ? 1 : 2; // Clockwise or counterclockwise
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
 * Computes the convex hull of a set of points using Graham's scan algorithm
 * Returns the number of points in the convex hull
 */
int convex_hull(Point points[], int n, Point hull[]) {
    // If there are less than 3 points, all points are part of the hull
    if (n < 3) {
        for (int i = 0; i < n; i++)
            hull[i] = points[i];
        return n;
    }

    // Sort points lexographically (first by x, then by y)
    qsort(points, n, sizeof(Point), compare_points);

    int k = 0; // Index for the hull array

    // Build lower hull
    for (int i = 0; i < n; i++) {
        // Remove points that would create a clockwise turn
        while (k >= 2 && orientation(hull[k-2], hull[k-1], points[i]) != 2)
            k--;
        hull[k++] = points[i];
    }

    // Build upper hull
    for (int i = n-2, t = k+1; i >= 0; i--) {
        // Remove points that would create a clockwise turn
        while (k >= t && orientation(hull[k-2], hull[k-1], points[i]) != 2)
            k--;
        hull[k++] = points[i];
    }

    return k-1; // The first point is repeated at the end
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

    // Allocate memory for convex hull (with extra space)
    Point *hull = (Point *)malloc(num_points * 2 * sizeof(Point));
    if (!hull) {
        printf("Memory allocation failed\n");
        free(points);
        return 1;
    }

    // Compute convex hull and its area
    int hull_size = convex_hull(points, num_points, hull);
    float area = calculate_polygon_area(hull, hull_size);
    
    // Print the area with one decimal place
    printf("%.1f\n", area);

    // Free allocated memory
    free(points);
    free(hull);
    
    return 0;
}