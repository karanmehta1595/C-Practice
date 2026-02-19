#include <stdio.h>

int main() {

    char Name[30];
    double Principal;
    double ROI;
    int Months;
    double Simple_Interest;
    double Total;

    printf("Enter Customer Name: ");
    scanf(" %29[^\n]", Name);

    printf("Enter Principal Amount: ");
    scanf("%lf", &Principal);

    printf("Enter Rate of Interest: ");
    scanf("%lf", &ROI);

    printf("Enter Tenure (Months): ");
    scanf("%d", &Months);

    // -------- Validation Section --------

    if (Principal <= 0) {
        printf("Invalid Principal Amount!\n");
        return 0;
    }

    if (ROI <= 0) {
        printf("Invalid Rate of Interest!\n");
        return 0;
    }

    if (Months <= 0) {
        printf("Invalid Tenure!\n");
        return 0;
    }

    if (ROI > 50) {
        printf("Warning: High Risk Interest Rate!\n");
    }

    // -------- Calculation --------

    Simple_Interest = (Principal * ROI * (Months / 12.0)) / 100;
    Total = Principal + Simple_Interest;

    // -------- Output --------

    printf("\n----------- Bank Interest Slip -----------\n");
    printf("Customer Name : %s\n", Name);
    printf("Principal     : %.2f\n", Principal);
    printf("Rate          : %.2f %%\n", ROI);
    printf("Tenure        : %d Months\n", Months);
    printf("------------------------------------------\n");
    printf("Interest Earned : %.2f\n", Simple_Interest);
    printf("Total Amount    : %.2f\n", Total);

    return 0;
}
