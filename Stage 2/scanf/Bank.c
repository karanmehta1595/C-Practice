#include <stdio.h>

int main() {
                char   Name[30];
                double Principle;
                double ROI;
                int    Months;
                double Simple_Intrest;
                double Total;
                
        printf("Enter Customer Name :");
        scanf(" %29[^\n]", Name);

        printf("Enter Principle Amount :");
        scanf("%lf", &Principle);
        
        printf("Enter Rate of Intrest :");
        scanf("%lf", &ROI);

        printf("Enter Tenure Months :");
        scanf("%d", &Months);

 Simple_Intrest = (Principle*ROI*(Months/12.0))/100;
 Total = Principle+Simple_Intrest;

printf("\n----------- Bank Interest Slip -----------\n");
printf("\nCustomer Name : %s\n", Name);
printf("Principle     : %.2f\n", Principle);
printf("Rate          : %.2f", ROI);
printf(" %% \n");
printf("Tenure        : %d", Months);
printf(" Months \n");
printf("\n-------------------------------------------\n");
printf("Intrest Earned: %.2f\n", Simple_Intrest);
printf("Total Amount  : %.2f\n", Total);

    return 0;
}