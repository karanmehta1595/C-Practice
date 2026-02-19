#include<stdio.h>

int main ()
{
    char Employee[30];
    double Basic;
    double HRA;
    double DA;
    double HRA_Amount;
    double DA_Amount;
    double Gross_Salary;

    printf("Enter Employee Name: ");
    scanf(" %29[^\n]", Employee);

    printf("Enter Basic Salary: ");
    scanf("%lf", &Basic);

    printf("Enter HRA (in %%): ");
    scanf("%lf", &HRA);

    printf("Enter DA (in %%): ");
    scanf("%lf", &DA);

    HRA_Amount = Basic * HRA / 100;
    DA_Amount = Basic * DA / 100;

    Gross_Salary = Basic + HRA_Amount + DA_Amount;

    printf("\n------ Salary Slip ------\n");
    printf("Employee Name: %s\n", Employee);
    printf("Basic Salary: %.2f\n", Basic);
    printf("HRA: %.2f\n", HRA_Amount);
    printf("DA: %.2f\n", DA_Amount);
    printf("Gross Salary is: %.2f\n", Gross_Salary);

    return 0;
}
        