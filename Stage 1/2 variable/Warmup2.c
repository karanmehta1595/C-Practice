#include <stdio.h>

int main() {
    int a = 5;
int b = 2;
double c = a/2.0;
printf("%lf", c);/*yahan hum answer ko sahi banayenge isko
compile krne par jawab galat aayega so lets make it correct
RN it's : #include <stdio.h>
int main() {
    int a = 5;
int b = 2;
double c = a/b;
printf("%lf", c);
return 0; now lets correct it*/
    return 0;
}