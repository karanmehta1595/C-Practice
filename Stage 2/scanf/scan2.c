#include <stdio.h>

int main()
{
    float number;
    char name[20];

    printf("Enter the number: ");
    scanf("%f", &number);

    printf("Enter the name: ");
    scanf("%19s", name);

    printf("Number: %.2f\n", number);
    printf("Name: %s\n", name);

    return 0;
}
