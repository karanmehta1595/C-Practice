#include<stdio.h>
int main()
{
    int x = 10;
    float y = 3.5;
    double z = 2.25;
    double sum = x*y+z/y;

    printf("Result=%.3lf\n", sum);
    return 0;
}