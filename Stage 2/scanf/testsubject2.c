#include <stdio.h>

int main() {
    char Consumer[20];
    char Address[100];
    double Phone;
    char Product[100];
    int Qty;
    double Price;

            printf("Enter Consumer Name:");
            scanf("%29s", Consumer);
            printf("Enter Consumer Address:");
            scanf("%99s", Address);
            printf("Eneter Phone Number:");
            scanf("%lf", &Phone);
            printf("Eneter Product Name:");
            scanf("%99s", Product);
            printf("Eneter Quantity:");
            scanf("%d", &Qty);
            printf("Eneter Price:");
            scanf("%lf", &Price);
            
        printf("Consumer Name: %s", Consumer);
        printf("Consumer Address: %s", Address);
        printf("Phone Number: %lf", Phone);
        printf("Product Name: %s", Product);
        printf("Quantity: %d", Qty);
        printf("Price: %lf", Price);
        printf("Total Bill Amount: %lf", Qty*Price);  
                                                    return 0;
                                                                }