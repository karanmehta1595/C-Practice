#include <stdio.h>

int main() {

    char Consumer[30];
    char Address[100];
    char Product[100];
    int Qty;
    double Price;
    
            printf("Name of Consumer:");
            scanf(" %29[^\n]", Consumer);

            printf("Address:");
            scanf(" %99[^\n]", Address);
            
            printf("Product Name:");
            scanf(" %29[^\n]", Product);
            
            printf("Quantity:");
            scanf(" %d", &Qty);
            
            printf("Price:");
            scanf(" %lf", &Price);
            
printf("----- BILL -----\n");
printf("Name of Consumer: %s\n", Consumer);
printf("Address: %s\n", Address);
printf("Product Name: %s\n", Product);
printf("Product Quantity: %d\n", Qty);
printf("Price of the Product: %.2lf\n", Price);
printf("Total Amount Paid: %.2lf\n", Qty*Price);



    
return 0;}