#include <stdio.h>

int main() {
    char Name[30];
    int Unit;
    double Rate;
    double Basic_Bill;
    double Surcharge;
    double Electricity_Tax;
    double Final_Bill;
    
printf("Consumer Name:");
scanf(" %29[^\n]", Name);
 
printf("Unit:");
scanf("%d", &Unit);

printf("Rate:");
scanf("%lf", &Rate);

Basic_Bill = Unit*Rate;
Surcharge = Basic_Bill*5/100;
Electricity_Tax = Basic_Bill*8/100;
Final_Bill = Basic_Bill+Surcharge+Electricity_Tax;


printf("\n----------     Electricity Bill   -------------------------\n");
printf("\nName of Consumer           : %s\n", Name);
printf("Unit Consumed              : %d\n", Unit);
printf("Rate per Unit              : %.2f\n", Rate);
printf("\n------------------------------------------------------------\n");
printf("Basic Bill                 : %.2f\n", Basic_Bill);
printf("Surcharge  (5%%)            : %.2f\n", Surcharge);
printf("Electricity Tax  (8%%)      : %.2f\n", Electricity_Tax);
printf("------------------------------------------------------------\n");
printf("Final Bill Amount          : %.2f\n", Final_Bill);


    return 0;
}