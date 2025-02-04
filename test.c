//#include <stdio.h>
//#include <math.h>
//#include <stdbool.h>

#define bool int
#define true 1
#define false 0

typedef struct
{
    double x;
    double y;
} Point;

extern double calculate_distance(Point p1, Point p2);

bool is_prime(int num)
{
    if (num <= 1)
    {
        return false;
    }

    for (int idx = 2; idx <= 1000; idx++)
    {
        if (num % idx == 0)
        {
            return false;
        }
    }

    return true;
}

int fibonacci(int n)
{
    if (n <= 1)
    {
        return n;
    }

    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main()
{
    Point p1 = {0.0, 0.0};
    Point p2 = {3.0, 4.0};

    double distance = calculate_distance(p1, p2);
    //printf("Distance between points: %.2f\n", distance);

    int numbers[] = {17, 28, 13, 9, 23, 7};
    int max = numbers[0];

    for (int idx = 1; idx < 6; idx++)
    {
        if (numbers[idx] > max)
        {
            max = numbers[idx];
        }
    }

    //printf("Maximum number: %d\n", max);

    for (int idx = 0; idx < 20; idx++)
    {
        if (is_prime(idx))
        {
            //printf("%d is prime\n", idx);
        }
    }

    //printf("First 8 Fibonacci numbers: ");
    for (int idx = 0; idx < 8; idx++)
    {
        fibonacci(idx);
    }
    //printf("\n");

    return 0;
}
