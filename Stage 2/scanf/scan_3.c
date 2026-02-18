#include <stdio.h>

int main() {
    double number;
    char Character[20];
    
        printf("Enter number:");
        scanf("%lf", &number);

        printf("Enter character:");
        scanf("%19s", Character);

    printf("You enterd number: %lf\n", number);
    printf("You enterd character: %s\n", Character);
    printf("Triple of number is: %lf\n", number*3);
    return 0;
}