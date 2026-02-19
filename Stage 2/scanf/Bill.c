#include<stdio.h>
int main()
{

char Name[30];
char Product[40];
int Qty;
double Price;

printf("Enter Name:");
scanf(" %29[^\n]", Name);

printf("Enter Product:");
scanf( " %39[^\n]", Product);

printf("Enter Quantity:");
scanf( "%d", &Qty);

printf("Enter Price:");
scanf( "%lf", &Price);

printf("----------Bill----------\n");
printf("Custmer Name: %s\n", Name);
printf("Product: %s\n", Product);
printf("Quantity: %d\n", Qty);
printf("Price: %.2f\n", Price);
printf("Total Bill Amount: %.2f\n", Price*Qty);

return 0;
}