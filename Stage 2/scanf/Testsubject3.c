#include <stdio.h>

int main() {
    char Consumer[30];
    char Address[100];
    char Phone[15];
    char Product[100];
    int Qty;
    double Price;

            printf("Enter Consumer Name:");
            scanf("%29s", Consumer);
            printf("Enter Consumer Address:");
            scanf("%99s", Address);
            printf("Enter Phone Number:");
            scanf("%14s", Phone);
            printf("Eneter Product Name:");
            scanf("%99s", Product);
            printf("Eneter Quantity:");
            scanf("%d", &Qty);
            printf("Eneter Price:");
            scanf("%lf", &Price);
            
        printf("Consumer Name: %s\n", Consumer);
        printf("Consumer Address: %s\n", Address);
        printf("Phone Number: %s\n", Phone);
        printf("Product Name: %s\n", Product);
        printf("Quantity: %d\n", Qty);
        printf("Price: %.2lf\n", Price);
        printf("Total Bill Amount: %.2lf\n", Qty*Price);  
                                                    return 0;
                                                                }