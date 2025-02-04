typedef struct
{
    double x;
    double y;
} Point;

double calculate_distance(Point p1, Point p2) __attribute__((used))
{
    double dx = p2.x - p1.x;
    double dy = p2.y - p1.y;

    return dx*dx+dy*dy;
}
