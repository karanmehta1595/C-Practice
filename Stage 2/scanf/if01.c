#include <stdio.h>

int main() {
    int age;

    printf("Enter Age :");
    scanf("%d", &age);

    if (age>=18)
    { printf("Congratulations! You can Vote.\n");
        /* code */
    }
        else 
    { 
        printf("Sorry! You Can't Vote.\n");
        /* code */
    }
    
    return 0;
}